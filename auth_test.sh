#!/bin/bash

# Simple authentication test
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Authentication Test ===${NC}"

# Ensure no servers are running
echo -e "${YELLOW}Cleaning up any previous processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Force rebuild all executables for system compatibility
echo -e "${YELLOW}Rebuilding executables for this system...${NC}"
make clean
make all

# Verify executables are compatible
echo -e "${YELLOW}Verifying executable compatibility...${NC}"
if ! file ./serverM | grep -q "executable" || ! file ./serverA | grep -q "executable"; then
    echo -e "${RED}Error: Executables are not compatible with this system!${NC}"
    echo -e "${YELLOW}Executables details:${NC}"
    file ./serverM ./serverA
    exit 1
fi

# Start only the servers needed for auth (M and A)
echo -e "${YELLOW}Starting Server M...${NC}"
./serverM > serverM.log 2>&1 &
M_PID=$!
sleep 2

echo -e "${YELLOW}Starting Server A...${NC}"
./serverA > serverA.log 2>&1 &
A_PID=$!
sleep 2

# Check if servers are running
if ! ps -p $M_PID > /dev/null || ! ps -p $A_PID > /dev/null; then
    echo -e "${RED}Error: Server M or A failed to start!${NC}"
    echo -e "Server M log:"
    cat serverM.log
    echo -e "Server A log:"
    cat serverA.log
    pkill -9 -f "server[MAPQ]" 2>/dev/null || true
    exit 1
fi

echo -e "${GREEN}Servers M and A started successfully!${NC}"

# Prepare test credentials
echo -e "${YELLOW}Creating test credentials...${NC}"
echo -e "user1\nsdvv789\nexit" > auth_test_input.txt

# Run the client with timeout
echo -e "${YELLOW}Running client...${NC}"
timeout 10 ./client < auth_test_input.txt > auth_test_output.txt 2>&1
CLIENT_EXIT=$?

echo -e "\n${YELLOW}Client Output:${NC}"
cat auth_test_output.txt

# Check result
if [ $CLIENT_EXIT -eq 124 ]; then
    echo -e "\n${RED}Test FAILED: Client timed out${NC}"
elif grep -q "You have been granted access" auth_test_output.txt; then
    echo -e "\n${GREEN}Test PASSED: Authentication successful${NC}"
else
    echo -e "\n${RED}Test FAILED: Authentication unsuccessful${NC}"
fi

# Show logs for debugging
echo -e "\n${YELLOW}Server M log:${NC}"
cat serverM.log
echo -e "\n${YELLOW}Server A log:${NC}"
cat serverA.log

# Clean up
echo -e "\n${YELLOW}Cleaning up server processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true

echo -e "\n${GREEN}Test completed!${NC}"