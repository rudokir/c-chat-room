#!/bin/bash

# Version information
VERSION="v1.1.0"
VERSION_FILE=".version"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Clear screen
clear

echo -e "${BLUE}"
cat << "EOF"
   ______ __          __     ____
  / ____// /_  ____ _/ /_   / __ \ ____   ____   ____ ___
 / /    / __ \/ __ '/ __/  / /_/ // __ \ / __ \ / __ '__ \
/ /___ / / / / /_/ / /_   / _, _// /_/ // /_/ // / / / / /
\____//_/ /_/\__,_/\__/  /_/ |_| \____/ \____//_/ /_/ /_/
EOF
echo -e "${NC}"

echo -e "${GREEN}Building Chat Application ${VERSION}${NC}\n"

# Check if version file exists and compare versions
if [ -f "$VERSION_FILE" ]; then
    OLD_VERSION=$(cat "$VERSION_FILE")
    if [ "$OLD_VERSION" != "$VERSION" ]; then
        echo -e "${YELLOW}Updating from ${OLD_VERSION} to ${VERSION}${NC}"
        echo -e "${YELLOW}Cleaning old binaries...${NC}"
        rm -f chat-server chat-client
    fi
fi

# Save current version
echo "$VERSION" > "$VERSION_FILE"

# Check if gcc is installed
if ! command -v gcc &> /dev/null; then
    echo -e "${RED}Error: gcc is not installed${NC}"
    echo "Please install gcc first:"
    echo "  Ubuntu/Debian: sudo apt install gcc"
    echo "  MacOS: xcode-select --install"
    echo "  CentOS/RHEL: sudo yum install gcc"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build

# Compile server with version information
echo -n "Compiling server... "
if gcc -Wall -Wextra -DVERSION=\"$VERSION\" chat-server.c -o build/chat-server; then
    echo -e "${GREEN}SUCCESS${NC}"
else
    echo -e "${RED}FAILED${NC}"
    exit 1
fi

# Compile client with version information
echo -n "Compiling client... "
if gcc -Wall -Wextra -DVERSION=\"$VERSION\" chat-client.c -o build/chat-client; then
    echo -e "${GREEN}SUCCESS${NC}"
else
    echo -e "${RED}FAILED${NC}"
    exit 1
fi

# Create symbolic links in root directory
ln -sf build/chat-server chat-server
ln -sf build/chat-client chat-client

echo -e "\n${GREEN}Build completed successfully!${NC}"
echo -e "Version: ${BLUE}${VERSION}${NC}"
echo -e "\nTo start the server:"
echo -e "${BLUE}./chat-server${NC}"
echo -e "\nTo start a client:"
echo -e "${BLUE}./chat-client${NC}"

# Make the compiled files executable
chmod +x build/chat-server build/chat-client

echo -e "\n${GREEN}Files are ready to execute!${NC}"

# Show build information
echo -e "\nBuild Information:"
echo -e "----------------"
echo -e "Date: $(date)"
echo -e "Version: ${VERSION}"
echo -e "Compiler: $(gcc --version | head -n1)"
echo -e "Platform: $(uname -s)"
echo -e "Architecture: $(uname -m)"
