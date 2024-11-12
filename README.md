# Chat Room Application using C

A robust command-line chat application written in C that supports multiple concurrent users, chat rooms, private messaging, and more.

## Version History

### v1.1.0 (Current)
- Added chat rooms functionality with lobby system
- Implemented room management (create, join, leave)
- Added automatic room cleanup when empty
- Enhanced private messaging system
- Added better buffer management and security
- Fixed various memory leaks and bugs

### v1.0.0
- Initial release
- Basic chat functionality
- Multiple client support
- Username identification
- Basic command system

## Features

- 🚀 Multi-client support (up to 10 simultaneous connections)
- 👤 Username identification and nickname changes
- 🏠 Multiple chat rooms with management
  - Default lobby system
  - Create custom rooms
  - Join/leave functionality
  - Automatic room cleanup
- 💬 Private messaging system
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
git clone https://github.com/yourusername/c-chat-app.git
cd c-chat-app
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

3. You will automatically join the lobby.

### Available Commands

- `/help` - Show available commands
- `/list` - List all connected users
- `/whois <username>` - Show information about a user
- `/nick <new_name>` - Change your nickname
- `/msg <user> <message>` - Send private message
- `/rooms` - List available chat rooms
- `/create <room>` - Create a new chat room
- `/join <room>` - Join a chat room
- `/leave` - Leave current room

## Chat Rooms System

### Default Lobby
- All users automatically join the lobby upon connection
- The lobby cannot be deleted
- Users can chat in the lobby like any other room

### Custom Rooms
- Created using `/create <room_name>`
- Automatically deleted when empty
- Maximum of 5 rooms at once (including lobby)
- Room creator automatically joins their created room

## Message Format

Messages appear in the following formats:

- Regular room messages:
```
[2024-03-12 14:30:45] [Room] John: Hello everyone!
```

- System messages:
```
[2024-03-12 14:30:45] SYSTEM: John has joined the chat
[2024-03-12 14:31:20] SYSTEM: Room Gaming has been closed (no active users)
```

- Private messages:
```
[2024-03-12 14:30:45] [PM from Alice]: Hey there!
[2024-03-12 14:30:45] [PM to Bob]: How are you?
```

## Technical Details

The application uses:
- TCP sockets for communication
- Poll for handling multiple clients
- POSIX-compliant C code
- System V networking primitives
- Dynamic memory management for rooms
- Secure buffer handling

### Security Features
- Protected against buffer overflows
- Secure string handling
- Memory leak prevention
- Proper error handling
- Input validation

## Limitations

- Maximum of 10 concurrent users
- Maximum of 5 chat rooms (including lobby)
- Username length limited to 31 characters
- Message length limited to 511 characters
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

Linkedin Profile - [@rudokir](https://linkedin.com/in/rudokir)

Project Link: [https://github.com/rudokir/c-chat-room](https://github.com/rudokir/c-chat-room)
