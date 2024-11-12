#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256
#define NAME_SIZE   32
#define PORT 9340

typedef struct {
	int fd;
	char name[NAME_SIZE];
	int slot_index;
} Client;

void broadcast_message(Client *clients, int sender_index, const char *message, int max_clients) {
	time_t now;
	time(&now);
	char timestamp[26];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

	char formatted_message[BUFFER_SIZE + 64];
	snprintf(formatted_message, sizeof(formatted_message), "[%s] %s: %s", timestamp, clients[sender_index].name, message);

	for (int i = 0; i < max_clients; i++) {
		if (clients[i].fd != -1 && i != sender_index) {
			send(clients[i].fd, formatted_message, strlen(formatted_message), 0);
		}
	}
}

void broadcast_system_message(Client *clients, const char *message, int max_clients) {
	time_t now;
	time(&now);
	char timestamp[26];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

	char formatted_message[BUFFER_SIZE + 64];
	snprintf(formatted_message, sizeof(formatted_message), "[%s] SYSTEM: %s", timestamp, message);

	for (int i = 0; i < max_clients; i++) {
		if (clients[i].fd != -1) {
			send(clients[i].fd, formatted_message, strlen(formatted_message), 0);
		}
	}
}

void clear_client_slot(Client *client) {
	client->fd = -1;
	memset(client->name, 0, NAME_SIZE);
	client->slot_index = -1;
}

int main () {
	int server_socket;
	struct sockaddr_in server_addr;
	Client clients[MAX_CLIENTS];
	struct pollfd fds[MAX_CLIENTS + 1]; // +1 for server socket
	int nfds = 1;

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
			int slot_found;
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (clients[i].fd == -1) {
					// Clear the slot completely before using
					clear_client_slot(&clients[i]);

					clients[i].fd = new_socket;
					clients[i].slot_index = nfds;

					// Receive client's name with proper buffer handling
					char name_buffer[NAME_SIZE] = {0};
					int bytes_received = recv(new_socket, name_buffer, NAME_SIZE - 1, 0);
					if (bytes_received > 0) {
						name_buffer[bytes_received] = '\0'; // Ensure null termination
						strncpy(clients[i].name, name_buffer, NAME_SIZE - 1);
						clients[i].name[NAME_SIZE - 1] = '\0'; // Ensure null termination
					} else {
						strncpy(clients[i].name, "Anonymous", NAME_SIZE - 1);
					}

					// Add to pollfd array
					fds[nfds].fd = new_socket;
					fds[nfds].events = POLLIN;
					fds[nfds].revents = 0;
					nfds++;

					// Broadcast join message
					char join_message[BUFFER_SIZE];
					snprintf(join_message, sizeof(join_message), "%s has joined the chat", clients[i].name);
					broadcast_system_message(clients, join_message, MAX_CLIENTS);
					
					printf("New connection: %s (socket: %d, slot: %d)\n", clients[i].name, new_socket, i);

					slot_found = 1;
					break;
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
					// Client disconnected
					char leave_message[BUFFER_SIZE];
					snprintf(leave_message, sizeof(leave_message), "%s has left the chat", clients[client_index].name);
					broadcast_system_message(clients, leave_message, MAX_CLIENTS);

					printf("Client disconnected: %s (socket: %d, slot: %d)\n", clients[client_index].name, clients[client_index].fd, client_index);

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
					buffer[bytes_received] = '\0'; // Ensure null termination
					broadcast_message(clients, client_index, buffer, MAX_CLIENTS);
				}
			}
		}
	}

	// Cleanup
	close(server_socket);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd != -1) {
			close(clients[i].fd);
		}
	}

	return 0;
}
