#!/bin/bash

# Quick test script that assumes servers are already running
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Quick Test ====${NC}"

# Check if servers are running
if ! pgrep -f "serverM" > /dev/null; then
    echo -e "${RED}Servers are not running. Please start the servers first using:"
    echo -e "./start_servers.sh${NC}"
    exit 1
fi

echo -e "${GREEN}Servers detected as running!${NC}"

# Test with automated commands
echo -e "${YELLOW}Running automated client tests...${NC}"
./client < test_commands.txt > test_output.txt

# Display test results
echo -e "\n${GREEN}Test Results:${NC}"
cat test_output.txt

echo -e "\n${GREEN}Test completed!${NC}"