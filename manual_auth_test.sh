#!/bin/bash

# Manual authentication test for interactive user verification
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Manual Authentication Test ===${NC}"

# Check if servers are running in workflow
if ! pgrep -f "serverM" > /dev/null; then
    echo -e "${RED}Servers are not running. Please start the servers first using the All Servers workflow.${NC}"
    echo -e "${YELLOW}You can run:\nworkflows restart \"All Servers\"${NC}"
    exit 1
fi

echo -e "${GREEN}Servers detected as running!${NC}"
echo -e "${YELLOW}Running client for manual testing...${NC}"
echo -e "${YELLOW}Use these test credentials:\nUsername: user1\nPassword: sdvv789${NC}"
echo -e "${YELLOW}After authentication, type 'exit' to close the client.${NC}"
echo -e "\n${GREEN}Press Enter to start the client...${NC}"
read

# Run the client for interactive testing
./client