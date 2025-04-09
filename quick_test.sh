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
    echo -e "${RED}Servers are not running.${NC}"
    echo -e "${YELLOW}Starting servers automatically...${NC}"
    
    # Force clean rebuild
    make clean
    make all
    
    # Verify executables
    if ! file ./serverM | grep -q "executable"; then
        echo -e "${RED}Error: Executables are not compatible with this system!${NC}"
        file ./serverM ./serverA ./serverP ./serverQ
        exit 1
    fi
    
    # Start servers
    ./serverM > serverM.log 2>&1 &
    sleep 1
    ./serverA > serverA.log 2>&1 &
    sleep 1
    ./serverP > serverP.log 2>&1 &
    sleep 1
    ./serverQ > serverQ.log 2>&1 &
    sleep 1
    
    if ! pgrep -f "serverM" > /dev/null; then
        echo -e "${RED}Failed to start servers. Check the logs for details.${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}Servers detected as running!${NC}"

# Test with automated commands
echo -e "${YELLOW}Running automated client tests...${NC}"
./client < test_commands.txt > test_output.txt

# Display test results
echo -e "\n${GREEN}Test Results:${NC}"
cat test_output.txt

echo -e "\n${GREEN}Test completed!${NC}"