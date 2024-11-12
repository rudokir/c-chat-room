# Simple C Chat Room Application

A simple yet robust command-line chat application written in C that supports multiple concurrent users, timestamps, and username identification.

## Features

- 🚀 Multi-client support (up to 10 simultaneous connections)
- 👤 Username identification
- 🕒 Message timestamps
- 📢 Join/Leave notifications
- 🔄 Automatic connection management
- ⚠️ Comprehensive error handling
- 🎨 System messages in a different format
- 🔒 Buffer overflow protection

## Prerequisites

To compile and run this chat application, you need:

- GCC compiler
- UNIX-like operating system (Linux, macOS, etc.)
- Basic knowledge of terminal/command line

## Installation

1. Clone the repository:
```bash
git clone https://github.com/rudokir/c-chat-room.git
cd c-chat-room
```

2. Build the application using the provided build script:
```bash
./build.sh
```

Or compile manually:
```bash
gcc -Wall -Wextra chat-server.c -o chat-server
gcc -Wall -Wextra chat-client.c -o chat-client
```

## Usage

### Starting the Server

1. Start the chat server:
```bash
./chat-server
```
The server will start listening on port 9340.

### Connecting Clients

1. In a new terminal window, start a chat client:
```bash
./chat-client
```

2. Enter your username when prompted.

3. Start chatting!

You can open multiple terminal windows to simulate different users connecting to the chat.

## Message Format

Messages in the chat appear in the following formats:

- Regular messages:
```
[2024-03-12 14:30:45] John: Hello everyone!
```

- System messages:
```
[2024-03-12 14:30:45] SYSTEM: John has joined the chat
[2024-03-12 14:31:20] SYSTEM: Alice has left the chat
```

## Technical Details

The application uses:
- TCP sockets for communication
- Poll for handling multiple clients
- POSIX-compliant C code
- System V networking primitives

### Server Features
- Handles up to 10 concurrent connections
- Manages client disconnections gracefully
- Broadcasts messages to all connected clients
- Maintains client information including usernames

### Client Features
- Username selection at startup
- Real-time message reception
- Clean disconnection handling
- Error reporting

## Error Handling

The application handles various error conditions:
- Server full (too many clients)
- Connection failures
- Unexpected disconnections
- Buffer overflows
- Socket errors

## Limitations

- Maximum of 10 concurrent users
- Username length limited to 31 characters
- Message length limited to 255 characters
- Local network usage only (can be modified for internet use)
- No message persistence (messages are not saved)

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by classic UNIX chat systems
- Built with modern error handling practices
- Designed for educational purposes

## Contact

Your Name - [@rudokir](https://linkedin.com/in/rudokir)

Project Link: [https://github.com/rudokir/c-chat-room](https://github.com/rudokir/c-chat-room)
