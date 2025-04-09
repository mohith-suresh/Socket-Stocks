#!/bin/bash

# Quick Start Script for Stock Trading System
# This script compiles the entire system, starts all servers,
# and runs a basic test to verify functionality

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color
BOLD='\033[1m'
NORMAL='\033[0m'

# Banner
echo -e "${YELLOW}${BOLD}===== Stock Trading System Quick Start =====${NC}${NORMAL}"
echo

# Step 1: Compile
echo -e "${YELLOW}Step 1: Compiling all components...${NC}"
make clean all

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed! Please fix errors and try again.${NC}"
    exit 1
fi

echo -e "${GREEN}Compilation successful!${NC}"
echo

# Step 2: Check for existing server processes
echo -e "${YELLOW}Step 2: Checking for existing servers...${NC}"

pgrep -f "serverM" > /dev/null
if [ $? -eq 0 ]; then
    echo -e "${YELLOW}Existing server processes found. Cleaning up...${NC}"
    pkill -f "server[MAPQ]"
    sleep 1
fi

echo -e "${GREEN}Server environment ready.${NC}"
echo

# Step 3: Start servers
echo -e "${YELLOW}Step 3: Starting all servers...${NC}"
chmod +x start_servers.sh
./start_servers.sh &
SERVER_PID=$!

# Wait for servers to start
echo -e "Waiting for servers to initialize..."
sleep 3

# Check if servers are running
pgrep -f "serverM" > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}Servers failed to start! Please check logs.${NC}"
    exit 1
fi

echo -e "${GREEN}All servers started successfully!${NC}"
echo

# Step 4: Run a quick test
echo -e "${YELLOW}Step 4: Running quick verification test...${NC}"
echo -e "Creating test input..."

cat > test_input.txt << EOF
user1
sdvv789
quote
exit
EOF

echo -e "Running client with test input..."
./client < test_input.txt > test_output.txt

if [ $? -ne 0 ]; then
    echo -e "${RED}Client test failed! Please check logs.${NC}"
    cat test_output.txt
    exit 1
fi

# Check for success indications in output
grep "granted access" test_output.txt > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}Authentication failed! Please check logs.${NC}"
    cat test_output.txt
    exit 1
fi

grep "S1 " test_output.txt > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}Quote retrieval failed! Please check logs.${NC}"
    cat test_output.txt
    exit 1
fi

echo -e "${GREEN}Verification test passed!${NC}"
echo

# Step 5: Instructions
echo -e "${YELLOW}${BOLD}===== System Ready! =====${NC}${NORMAL}"
echo
echo -e "The Stock Trading System is now running and ready to use."
echo
echo -e "${BOLD}To use the system:${NORMAL}"
echo -e "1. Run the client: ./client"
echo -e "2. Log in with credentials:"
echo -e "   - Username: user1"
echo -e "   - Password: sdvv789"
echo
echo -e "${BOLD}Available commands:${NORMAL}"
echo -e "- quote               Show all stock prices"
echo -e "- quote <stock>       Show specific stock price (e.g., quote S1)"
echo -e "- buy <stock> <shares> Buy shares of a stock (e.g., buy S1 5)"
echo -e "- sell <stock> <shares> Sell shares of a stock (e.g., sell S1 2)"
echo -e "- position            View your current portfolio"
echo -e "- exit                Logout and exit"
echo
echo -e "${YELLOW}To run specific tests, use the test scripts:${NC}"
echo -e "- ./quick_test.sh"
echo -e "- ./auth_only_test.sh"
echo -e "- ./quote_test.sh"
echo
echo -e "${YELLOW}To stop the servers:${NC}"
echo -e "Press Ctrl+C in the terminal where start_servers.sh is running"
echo -e "or run: pkill -f \"server[MAPQ]\""
echo
echo -e "${GREEN}${BOLD}Enjoy using the Stock Trading System!${NC}${NORMAL}"