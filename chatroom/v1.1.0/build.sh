#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
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

echo -e "${GREEN}Building Chat Application...${NC}\n"

# Check if gcc is installed
if ! command -v gcc &> /dev/null; then
    echo -e "${RED}Error: gcc is not installed${NC}"
    echo "Please install gcc first:"
    echo "  Ubuntu/Debian: sudo apt install gcc"
    echo "  MacOS: xcode-select --install"
    echo "  CentOS/RHEL: sudo yum install gcc"
    exit 1
fi

# Compile server
echo -n "Compiling server... "
if gcc -Wall -Wextra chat-server.c -o chat-server; then
    echo -e "${GREEN}SUCCESS${NC}"
else
    echo -e "${RED}FAILED${NC}"
    exit 1
fi

# Compile client
echo -n "Compiling client... "
if gcc -Wall -Wextra chat-client.c -o chat-client; then
    echo -e "${GREEN}SUCCESS${NC}"
else
    echo -e "${RED}FAILED${NC}"
    exit 1
fi

echo -e "\n${GREEN}Build completed successfully!${NC}"
echo -e "\nTo start the server:"
echo -e "${BLUE}./chat-server${NC}"
echo -e "\nTo start a client:"
echo -e "${BLUE}./chat-client${NC}"

# Make the compiled files executable
chmod +x chat-server chat-client

echo -e "\n${GREEN}Files are ready to execute!${NC}"
