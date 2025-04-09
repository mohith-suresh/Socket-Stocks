#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Client Test ====${NC}"
echo -e "${YELLOW}Testing client login with preset credentials${NC}"

# Ensure all servers are running (assumes start_servers.sh is already running)
echo -e "${YELLOW}Note: This script assumes all servers are already running${NC}"
echo -e "${YELLOW}If servers are not running, please run ./start_servers.sh first${NC}"

# Authenticate with user1
echo -e "\n${YELLOW}Testing authentication with user1/sdvv789...${NC}"
echo -e "This will connect to Main Server and authenticate with Auth Server"
echo -e "Expected result: Authentication successful\n"

echo -e "${GREEN}Press Enter to run test, or Ctrl+C to exit${NC}"
read

# Display command for clarity
echo -e "${YELLOW}Running: ./client (then entering user1 and sdvv789)${NC}"
./client 

echo -e "\n${YELLOW}Test complete.${NC}"
echo -e "If authentication was successful, you should have seen 'You have been granted access'"
echo -e "You can continue testing with the open client or exit the client"