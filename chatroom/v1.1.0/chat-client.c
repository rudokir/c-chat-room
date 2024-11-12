#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 256
#define NAME_SIZE   32
#define PORT	    9340

void error_exit(const char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}

void print_welcome_message() {
    printf("\n=== Welcome to the Chat Room === \n");
    printf("\tAvailable commands:\n");
    printf("/help\t- Show available commands\n");
    printf("/list\t- List connected users\n");
    printf("/whois <username> - Show user information\n");
    printf("/nick <new_name> - Change your nickname\n");
    printf("/msg <user> <message> - Send private message\n");
    printf("/rooms\t- List available chat rooms\n");
    printf("/create <room_name> - Create a new chat room\n");
    printf("/join <room_name> - Join a chat room\n");
    printf("/leave\t- Leave current chat room\n");
    printf("==========================\n\n");
}

int main () {
	int sockfd;
	struct sockaddr_in server_addr;
	char name[NAME_SIZE];

	// Get username
	printf("Enter your name (max %d characters): ", NAME_SIZE - 1);
	fgets(name, NAME_SIZE - 1, stdin);
	name[strcspn(name, "\n")] = 0; // Remove newline
	
	// Create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		error_exit("Socket creation failed");
	}

	// Configure server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(PORT);

	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		error_exit("Connection failed");
	}

	// Send username to server
	if (send(sockfd, name, strlen(name), 0) == -1) {
		error_exit("Failed to send username");
	}

    print_welcome_message();

	printf("Connected to chat server. Type your messages:\n");

	struct pollfd fds[2] = {
		{STDIN_FILENO, POLLIN, 0},
		{sockfd, POLLIN, 0}
	};

	char buffer[BUFFER_SIZE];

	for (;;) {
		int poll_result = poll(fds, 2, -1);

		if (poll_result == -1) {
			error_exit("Poll failed");
		}

		// Check for user input
		if (fds[0].revents & POLLIN) {
			memset(buffer, 0, BUFFER_SIZE);
			if (fgets(buffer, BUFFER_SIZE - 1, stdin) == NULL) {
				break;
			}
			buffer[strcspn(buffer, "\n")] = 0; // Remove newline

			if (strlen(buffer) > 0) {
				if (send(sockfd, buffer, strlen(buffer), 0) == -1) {
					error_exit("Failed to send message");
				}
			}
		}
		
		// Check for server messages
		if (fds[1].revents & POLLIN) {
			memset(buffer, 0, BUFFER_SIZE);
			int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);

			if (bytes_received <= 0) {
				printf("\nDisconnected from server\n");
				break;
			}

			printf("%s\n", buffer);
		}
	}
	
	close(sockfd);
	return 0;
}
