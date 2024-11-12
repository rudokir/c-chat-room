#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>

#define MAX_CLIENTS 10
#define MAX_ROOMS 5
#define BUFFER_SIZE 512
#define NAME_SIZE 32
#define PORT 9340
#define MAX_COMMAND_LENGTH 32
#define MAX_COMMAND_PARAMS 5
#define ROOM_NAME_SIZE 32
#define DEFAULT_ROOM "Lobby"

typedef struct {
    int fd;
    char name[NAME_SIZE];
    int slot_index;
    char *current_room;
} Client;

typedef struct {
    char name[ROOM_NAME_SIZE];
    int user_count;
    int active;
    int is_default;
} ChatRoom;

typedef struct {
    const char *name;
    const char *description;
    void (*handler)(Client *sender, Client *clients, ChatRoom *rooms, char *params);
} Command;

// Forward declarations of command handlers
void handle_help(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_list(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_whois(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_nick(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_msg(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_create(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_join(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_leave(Client *sender, Client *clients, ChatRoom *rooms, char *params);
void handle_rooms(Client *sender, Client *clients, ChatRoom *rooms, char *params);

// Global commands array
Command commands[] = {
    {"/help", "Show available commands", handle_help},
    {"/list", "List all connected users", handle_list},
    {"/whois", "Show information about a user", handle_whois},
    {"/nick", "Change your nickname", handle_nick},
    {"/msg", "Send private message: /msg <user> <message>", handle_msg},
    {"/create", "Create a new chat room: /create <room_name>", handle_create},
    {"/join", "Join a chat room: /join <room_name>", handle_join},
    {"/leave", "Leave current chat room", handle_leave},
    {"/rooms", "List all available chat rooms", handle_rooms},
    {NULL, NULL, NULL} // Terminator
};

void safe_strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0) return;
    
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
}

void send_to_client(Client *client, const char *message) {
    if (!client || client->fd == -1) return;

    char formatted[BUFFER_SIZE];
    time_t now;
    time(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M", localtime(&now));
    
    size_t written = snprintf(formatted, sizeof(formatted), "[%s] %s\n", timestamp, message);
    if (written < sizeof(formatted)) {
        send(client->fd, formatted, strlen(formatted), 0);
    }
}

void broadcast_to_room(Client *clients, ChatRoom *rooms __attribute__((unused)), Client *sender, const char *room_name, const char *message) {
    if (!room_name || ! message) return;
    
    time_t now;
    time(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M", localtime(&now));
    
    char formatted_message[BUFFER_SIZE];
    size_t written = snprintf(formatted_message, sizeof(formatted_message), "[%s] [%s] %s: %s\n", timestamp, room_name, sender->name, message);

    if (written < sizeof(formatted_message)) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && clients[i].current_room && strcmp(clients[i].current_room, room_name) == 0) {
                send(clients[i].fd, formatted_message, strlen(formatted_message), 0);
            }
        }
    }
}

void broadcast_system_message(Client *clients, const char *message, int max_clients) {
    time_t now;
    time(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    char formatted_message[BUFFER_SIZE + 64];
    snprintf(formatted_message, sizeof(formatted_message), "[%s] SYSTEM: %s\n", 
             timestamp, message);

    for (int i = 0; i < max_clients; i++) {
        if (clients[i].fd != -1) {
            send(clients[i].fd, formatted_message, strlen(formatted_message), 0);
        }
    }
}

// Command Handlers
void handle_help(Client *sender, Client *clients __attribute__((unused)), ChatRoom *rooms __attribute__((unused)), char *params __attribute__((unused))) {
    char help_message[BUFFER_SIZE * 4] = "Available commands:\n";
    for (int i = 0; commands[i].name != NULL; i++) {
        char cmd_info[BUFFER_SIZE];
        snprintf(cmd_info, sizeof(cmd_info), "%s - %s\n", 
                 commands[i].name, commands[i].description);
        strcat(help_message, cmd_info);
    }
    send_to_client(sender, help_message);
}

void handle_rooms(Client *sender, Client *clients __attribute__((unused)), ChatRoom *rooms, char *params __attribute__((unused))) {
    char room_list[BUFFER_SIZE * 4] = "Available rooms:\n";
    int room_count = 0;
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active) {
            char room_info[BUFFER_SIZE];
            snprintf(room_info, sizeof(room_info), "- %s (%d users)%s\n", rooms[i].name, rooms[i].user_count, rooms[i].is_default ? " [Default]" : "");
            strcat(room_list, room_info);
            room_count++;
        }
    }
    
    if (room_count == 0) {
        strcat(room_list, "No active rooms except the lobby.\n");
    }
    
    send_to_client(sender, room_list);
}

void handle_create(Client *sender, Client *clients, ChatRoom *rooms, char *params) {
    if (!params || strlen(params) == 0) {
        send_to_client(sender, "Usage: /create <room_name>");
        return;
    }
    
    // Check if room already exists
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && strcasecmp(rooms[i].name, params) == 0) {
            send_to_client(sender, "Room already exists.");
            return;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].active) {
            safe_strncpy(rooms[i].name, params, ROOM_NAME_SIZE - 1);
            rooms[i].name[ROOM_NAME_SIZE - 1] = '\0';
            rooms[i].user_count = 0;
            rooms[i].active = 1;
            
            char system_message[BUFFER_SIZE];
            snprintf(system_message, sizeof(system_message), 
                     "New room created: %s", rooms[i].name);
            broadcast_system_message(clients, system_message, MAX_CLIENTS);
            
            // Automatically join the created room
            char join_params[ROOM_NAME_SIZE];
            safe_strncpy(join_params, params, ROOM_NAME_SIZE - 1);
            handle_join(sender, clients, rooms, join_params);
            return;
        }
    }
    
    send_to_client(sender, "Maximum number of rooms reached.");
}

void handle_join(Client *sender, Client *clients, ChatRoom *rooms, char *params) {
    if (!params || strlen(params) == 0) {
        send_to_client(sender, "Usage: /join <room_name>");
        return;
    }
    
    // First leave current room if in one
    if (sender->current_room) {
        handle_leave(sender, clients, rooms, NULL);
    }
    
    // Find and join room
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && strcasecmp(rooms[i].name, params) == 0) {
            sender->current_room = strdup(rooms[i].name);
            rooms[i].user_count++;
            
            char system_message[BUFFER_SIZE];
            snprintf(system_message, sizeof(system_message), 
                     "%s joined room: %s", sender->name, rooms[i].name);
            broadcast_system_message(clients, system_message, MAX_CLIENTS);
            return;
        }
    }
    
    send_to_client(sender, "Room not found.");
}

void handle_leave(Client *sender, Client *clients, ChatRoom *rooms, char *params __attribute__((unused))) {
    if (!sender->current_room) {
        send_to_client(sender, "You are not in any room.");
        return;
    }
    
    // Find room and update count
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && strcmp(rooms[i].name, sender->current_room) == 0) {
            rooms[i].user_count--;
            
            char system_message[BUFFER_SIZE];
            snprintf(system_message, sizeof(system_message), 
                     "%s left room: %s", sender->name, rooms[i].name);
            broadcast_system_message(clients, system_message, MAX_CLIENTS);
            
            // If room is empty, deactivate it
            if (rooms[i].user_count == 0) {
                rooms[i].active = 0;
                snprintf(system_message, sizeof(system_message), 
                         "Room %s has been closed (no active users)", rooms[i].name);
                broadcast_system_message(clients, system_message, MAX_CLIENTS);
            }
            
            free(sender->current_room);
            sender->current_room = NULL;
            return;
        }
    }
}

void handle_msg(Client *sender, Client *clients, ChatRoom *rooms __attribute__((unused)), char *params) {
    if (!params || strlen(params) == 0) {
        send_to_client(sender, "Usage: /msg <username> <message>");
        return;
    }
    
    char target_name[NAME_SIZE];
    char message[BUFFER_SIZE];
    
    // Split params into target and message
    if (sscanf(params, "%s %[^\n]", target_name, message) != 2) {
        send_to_client(sender, "Usage: /msg <username> <message>");
        return;
    }
    
    // Find target client and send message
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && strcasecmp(clients[i].name, target_name) == 0) {
            const size_t header_size = 32;
            const size_t max_content_size = BUFFER_SIZE - header_size - NAME_SIZE - 5;

            char pm_content[BUFFER_SIZE];
            safe_strncpy(pm_content, message, max_content_size);
            pm_content[max_content_size] = '\0';

            char msg_to_recipient[BUFFER_SIZE];
            char msg_to_sender[BUFFER_SIZE];

            snprintf(msg_to_recipient, BUFFER_SIZE, "[PM from %.*s]: %.*s", NAME_SIZE - 1, sender->name, (int)max_content_size, pm_content);
            
            snprintf(msg_to_sender, BUFFER_SIZE, "[PM to %.*s]: %.*s", NAME_SIZE - 1, target_name, (int)max_content_size, pm_content);

            send_to_client(&clients[i], msg_to_recipient);
            send_to_client(sender, msg_to_sender);
            return;
        }
    }
    
    send_to_client(sender, "User not found.");
}

void handle_list(Client *sender, Client *clients, ChatRoom *rooms __attribute__((unused)), char *params __attribute__((unused))) {
    char list_message[BUFFER_SIZE * 4] = "Connected users:\n";
    int count = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            char user_info[BUFFER_SIZE];
            snprintf(user_info, sizeof(user_info), "- %s\n", clients[i].name);
            strcat(list_message, user_info);
            count++;
        }
    }

    char summary[64];
    snprintf(summary, sizeof(summary), "\nTotal users: %d\n", count);
    strcat(list_message, summary);

    send_to_client(sender, list_message);
}

void handle_whois(Client *sender, Client *clients, ChatRoom *rooms __attribute__((unused)), char *params) {
    if (!params || strlen(params) == 0) {
        send_to_client(sender, "Usage: /whois <username>");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && strcasecmp(clients[i].name, params) == 0) {
            char info[BUFFER_SIZE];
            snprintf(info, sizeof(info), "User: %s\nConnection ID: %d", clients[i].name, clients[i].slot_index);
            send_to_client(sender, info);
            return;
        }
    }

    send_to_client(sender, "User not found.");
}

void handle_nick(Client *sender, Client *clients, ChatRoom *rooms __attribute__((unused)), char *params) {
    if (!params || strlen(params) == 0) {
        send_to_client(sender, "Usage: /nick <new_nickname>");
        return;
    }

    // Check if nickname is already taken
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && strcasecmp(clients[i].name, params) == 0) {
            send_to_client(sender, "This nickname is already taken.");
            return;
        }
    }

    char old_name[NAME_SIZE];
    safe_strncpy(old_name, sender->name, NAME_SIZE - 1);

    safe_strncpy(sender->name, params, NAME_SIZE - 1);
    sender->name[NAME_SIZE - 1] = '\0';

    char system_message[BUFFER_SIZE];
    snprintf(system_message, sizeof(system_message), "%s has changed their name to %s", old_name, sender->name);
    broadcast_system_message(clients, system_message, MAX_CLIENTS);
}

int process_command(Client *sender, Client *clients, ChatRoom *rooms, char *message) {
    if (message[0] != '/') return 0;

    char cmd[MAX_COMMAND_LENGTH] = {0};
    char params[BUFFER_SIZE] = {0};

    // Split command and parameters
    sscanf(message, "%s %[^\n]", cmd, params);

    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcasecmp(cmd, commands[i].name) == 0) {
            commands[i].handler(sender, clients, rooms, params);
            return 1;
        }
    }

    send_to_client(sender, "Unknown command. Type /help for available commands.");
    return 1;
}

void clear_client_slot(Client *client) {
	client->fd = -1;
	memset(client->name, 0, NAME_SIZE);
	client->slot_index = -1;
    if (client->current_room) {
        free(client->current_room);
        client->current_room = NULL;
    }
}

void init_chat_rooms(ChatRoom *rooms) {
    if (!rooms) return;

	for (int i = 0; i < MAX_ROOMS; i++) {
		memset(&rooms[i], 0, sizeof(ChatRoom));
        rooms[i].user_count = 0;
        rooms[i].active = 0;
        rooms[i].is_default = 0;
    }

    safe_strncpy(rooms[0].name, DEFAULT_ROOM, ROOM_NAME_SIZE - 1);
    rooms[0].name[ROOM_NAME_SIZE - 1] = '\0';
    rooms[0].active = 1;
    rooms[0].is_default = 1;
    rooms[0].user_count = 0;
}

void init_client(Client *client, int fd, int slot_index, const char *name) {
    if (!client || !name) return;

    client->fd = fd;
    client->slot_index = slot_index;
    client->current_room = NULL;

    memset(client->name, 0, NAME_SIZE);
    safe_strncpy(client->name, name, NAME_SIZE - 1);
    client->name[NAME_SIZE - 1] = '\0';

    client->current_room = strdup(DEFAULT_ROOM);
    if (!client->current_room) {
        fprintf(stderr, "Failed to allocate memeory for room name\n");
        exit(EXIT_FAILURE);
    }
}

void handle_client_disconnect(Client *client, Client *clients, ChatRoom *rooms) {
    if (!client || !clients || !rooms || !client->current_room) return;

    if (client->current_room) {
        // Find and update room user count
        for (int i = 0; i < MAX_ROOMS; i++) {
            if (rooms[i].active && strcmp(rooms[i].name, client->current_room) == 0) {
                rooms[i].user_count--;

                // If room is empty and not default, close it
                if (rooms[i].user_count == 0 && !rooms[i].is_default) {
                    char system_message[BUFFER_SIZE];
                    snprintf(system_message, sizeof(system_message), "Room %s has been closed (no active users)", rooms[i].name);
                    broadcast_system_message(clients, system_message, MAX_CLIENTS);
                    rooms[i].active = 0;
                }
                break;
            }
        }
    }
}

void signal_handler(int signum) {
    fprintf(stderr, "Signal %d received\n", signum);
    exit(1);
}

int main () {
	int server_socket;
	struct sockaddr_in server_addr;
	Client clients[MAX_CLIENTS] = {0};
	struct pollfd fds[MAX_CLIENTS + 1] = {0}; // +1 for server socket
    ChatRoom chat_rooms[MAX_ROOMS] = {0};
	int nfds = 1;

    signal(SIGSEGV, signal_handler);

    // Initialize rooms
    init_chat_rooms(chat_rooms);

	// Initialize client array
	for (int i = 0; i < MAX_CLIENTS; i++) {
		clear_client_slot(&clients[i]);
	}

	// Create socket
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	// Set socket options to reuse address
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}

	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// Bind socket
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	// Listen for connections
	if (listen(server_socket, 10) == -1) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	printf("Chat server started on port %d\n", PORT);

	// Initialize the first pollfd structure for the server socket
	memset(fds, 0, sizeof(fds));
	fds[0].fd = server_socket;
	fds[0].events = POLLIN;
	
	for (;;) {
        int poll_result = poll(fds, nfds, -1);

        if (poll_result == -1) {
            perror("Poll failed");
            continue;
        }

        // Check for new connections
        if (fds[0].revents & POLLIN) {
            int new_socket = accept(server_socket, NULL, NULL);
            if (new_socket == -1) {
                perror("Accept failed");
                continue;
            }

            // Find an empty slot
            int slot_found = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    char name_buffer[NAME_SIZE] = {0};
                    int bytes_received = recv(new_socket, name_buffer, NAME_SIZE - 1, 0);
                    
                    if (bytes_received > 0) {
                        name_buffer[bytes_received] = '\0';

                        int name_exits = 0;
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].fd != -1 && strcasecmp(clients[j].name, name_buffer) == 0) {
                                name_exits = 1;
                                break;
                            }
                        }

                        if (name_exits) {
                            char reject_msg[] = "Username already taken\n";
                            send(new_socket, reject_msg, strlen(reject_msg), 0);
                            close(new_socket);
                            continue;
                        }

                        init_client(&clients[i], new_socket, nfds, name_buffer);
                        if (clients[i].current_room == NULL) {
                            fprintf(stderr, "Failed to initialize client room\n");
                            close(new_socket);
                            clear_client_slot(&clients[i]);
                            continue;
                        }

                        // Update lobby count
                        chat_rooms[0].user_count++; // Lobby is always at index 0
                        
                        // Add to pollfd array
                        fds[nfds].fd = new_socket;
                        fds[nfds].events = POLLIN;
                        fds[nfds].revents = 0;
                        nfds++;

                        // Welcome messages
                        char welcome_msg[BUFFER_SIZE];
                        snprintf(welcome_msg, sizeof(welcome_msg), "Welcome %s! You are now in the %s", clients[i].name, DEFAULT_ROOM);
                        send_to_client(&clients[i], welcome_msg);

                        char join_message[BUFFER_SIZE];
                        snprintf(join_message, sizeof(join_message), "%s has joined the %s", clients[i].name, DEFAULT_ROOM);
                        broadcast_system_message(clients, join_message, MAX_CLIENTS);

                        printf("New connection: %s (socket: %d, slot: %d)\n", clients[i].name, new_socket, i);
                        
                        slot_found = 1;
                        break;
                    }
                }
            }

            if (!slot_found) {
                printf("Server full, connection rejected\n");
                close(new_socket);
            }
        }

        // Check for client messages
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                char buffer[BUFFER_SIZE] = {0};
                int client_index = -1;

                // Find corresponding client
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].fd == fds[i].fd) {
                        client_index = j;
                        break;
                    }
                }

                if (client_index == -1) continue;

                int bytes_received = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);

                if (bytes_received <= 0) {
                    // Handle disconnection
                    char leave_message[BUFFER_SIZE];
                    snprintf(leave_message, sizeof(leave_message), "%s has left the chat", clients[client_index].name);
                    broadcast_system_message(clients, leave_message, MAX_CLIENTS);

                    printf("Client disconnected: %s (socket: %d, slot: %d)\n", clients[client_index].name, clients[client_index].fd, client_index);

                    // Update room status before clearing client
                    handle_client_disconnect(&clients[client_index], clients, chat_rooms);

                    close(fds[i].fd);

                    // Remove from pollfd array
                    for (int j = i; j < nfds - 1; j++) {
                        fds[j] = fds[j + 1];
                        // Update slot_index for affected clients
                        for (int k = 0; k < MAX_CLIENTS; k++) {
                            if (clients[k].slot_index == j + 1) {
                                clients[k].slot_index--;
                            }
                        }
                    }
                    nfds--;

                    // Clear client slot
                    clear_client_slot(&clients[client_index]);
                    i--;
                } else {
                    buffer[bytes_received] = '\0';
                    if (!process_command(&clients[client_index], clients, chat_rooms, buffer)) {
                        if (clients[client_index].current_room) {
                            broadcast_to_room(clients, chat_rooms, &clients[client_index], clients[client_index].current_room, buffer);
                        } else {
                            send_to_client(&clients[client_index], "Join a room first using /join <room_name>");
                        }
                    }
                }
            }
        }
    }

    // Cleanup
    close(server_socket);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
            if (clients[i].current_room) {
                free(clients[i].current_room);
            }
        }
    }

    return 0;
}
