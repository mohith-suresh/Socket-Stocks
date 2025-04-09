#!/bin/bash

# Define colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Authentication Debugging ====${NC}"

# Clean up previous processes
echo -e "${YELLOW}Cleaning up any previous server processes...${NC}"
pkill -9 -f "./serverM" 2>/dev/null || true
pkill -9 -f "./serverA" 2>/dev/null || true
pkill -9 -f "./serverP" 2>/dev/null || true
pkill -9 -f "./serverQ" 2>/dev/null || true
echo -e "${GREEN}Done cleaning up processes${NC}"

# Clean any previous log files
echo -e "${YELLOW}Cleaning log files...${NC}"
rm -f serverM.log serverA.log serverP.log serverQ.log
echo -e "${GREEN}Done cleaning logs${NC}"

# Verify the members.txt file
echo -e "${YELLOW}Checking members.txt file...${NC}"
if [ -f members.txt ]; then
    echo -e "${GREEN}members.txt exists:${NC}"
    cat members.txt
else
    echo -e "${RED}members.txt not found!${NC}"
    exit 1
fi

# Start only the needed servers
echo -e "\n${YELLOW}Starting minimal servers for auth test...${NC}"

# Start Server M in debug mode
echo -e "${YELLOW}Starting Server M...${NC}"
./serverM > serverM.log 2>&1 &
SERVER_M_PID=$!
echo -e "${GREEN}Server M started with PID $SERVER_M_PID${NC}"
sleep 1

# Start Server A
echo -e "${YELLOW}Starting Server A...${NC}"
./serverA > serverA.log 2>&1 &
SERVER_A_PID=$!
echo -e "${GREEN}Server A started with PID $SERVER_A_PID${NC}"
sleep 2

# Make sure both servers are running
echo -e "${YELLOW}Checking if servers are running...${NC}"
if ps -p $SERVER_M_PID > /dev/null && ps -p $SERVER_A_PID > /dev/null; then
    echo -e "${GREEN}Both servers are running${NC}"
else
    echo -e "${RED}One or more servers failed to start${NC}"
    exit 1
fi

# Create test input file
echo -e "${YELLOW}Creating test input...${NC}"
echo -e "user1\nsdvv789\nexit" > debug_auth_input.txt
echo -e "${GREEN}Test input created:${NC}"
cat debug_auth_input.txt

# Run the client
echo -e "\n${YELLOW}Running client with test input...${NC}"
timeout 10 ./client < debug_auth_input.txt > debug_auth_output.txt
CLIENT_EXIT_CODE=$?
echo -e "${GREEN}Client exited with code $CLIENT_EXIT_CODE${NC}"

# Check client output
echo -e "\n${YELLOW}Client output:${NC}"
cat debug_auth_output.txt

# Check server logs
echo -e "\n${YELLOW}Server M log:${NC}"
cat serverM.log

echo -e "\n${YELLOW}Server A log:${NC}"
cat serverA.log

# Clean up
echo -e "\n${YELLOW}Cleaning up server processes...${NC}"
kill -9 $SERVER_M_PID $SERVER_A_PID 2>/dev/null || true
echo -e "${GREEN}All servers stopped${NC}"

echo -e "\n${YELLOW}Summary:${NC}"
if grep -q "You have been granted access" debug_auth_output.txt || grep -q "Authentication successful" serverM.log; then
    echo -e "${GREEN}✓ Authentication Successful!${NC}"
else
    echo -e "${RED}✗ Authentication Failed.${NC}"
    
    # Additional diagnostics
    echo -e "\n${YELLOW}Diagnostics:${NC}"
    if ! ps -p $SERVER_M_PID > /dev/null || ! ps -p $SERVER_A_PID > /dev/null; then
        echo -e "${RED}- One or more servers crashed during the test${NC}"
    fi
    
    if grep -q "Could not open members file" serverA.log; then
        echo -e "${RED}- Server A couldn't open members.txt${NC}"
    fi
    
    if ! grep -q "Loaded .* user credentials" serverA.log; then
        echo -e "${RED}- Server A failed to load credentials${NC}"
    fi
    
    if ! grep -q "AUTH user1" serverA.log; then
        echo -e "${RED}- Server A never received auth request from Server M${NC}"
    fi
    
    if ! grep -q "Received authentication request" serverM.log; then
        echo -e "${RED}- Server M never received auth request from client${NC}"
    fi
fi

echo -e "\n${GREEN}Authentication test completed!${NC}"