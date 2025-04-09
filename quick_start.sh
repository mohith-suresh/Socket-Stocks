#!/bin/bash

# Quick Start Script for Stock Trading System
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}    Stock Trading System Quick Start    ${NC}"
echo -e "${BLUE}========================================${NC}"

# Create test_results directory if it doesn't exist
mkdir -p test_results

# Step 1: Clean up any previous processes
echo -e "\n${YELLOW}Step 1: Cleaning up any previous processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Step 2: Force rebuild all executables for system compatibility
echo -e "\n${YELLOW}Step 2: Rebuilding executables for this system...${NC}"
make clean
make all

# Step 3: Verify executables are compatible with this system
echo -e "\n${YELLOW}Step 3: Verifying executable compatibility...${NC}"
if ! file ./serverM | grep -q "executable" || ! file ./serverA | grep -q "executable"; then
    echo -e "${RED}Error: Executables are not compatible with this system!${NC}"
    echo -e "${YELLOW}Executables details:${NC}"
    file ./serverM ./serverA ./serverP ./serverQ ./client
    exit 1
fi
echo -e "${GREEN}✓ All executables are compatible with this system${NC}"

# Step 4: Start all servers
echo -e "\n${YELLOW}Step 4: Starting all servers...${NC}"
./serverM > serverM.log 2>&1 &
M_PID=$!
sleep 1
./serverA > serverA.log 2>&1 &
A_PID=$!
sleep 1
./serverP > serverP.log 2>&1 &
P_PID=$!
sleep 1
./serverQ > serverQ.log 2>&1 &
Q_PID=$!
sleep 1

# Step 5: Check if all servers are running
echo -e "\n${YELLOW}Step 5: Verifying all servers are running...${NC}"
if ps -p $M_PID > /dev/null && ps -p $A_PID > /dev/null && 
   ps -p $P_PID > /dev/null && ps -p $Q_PID > /dev/null; then
    echo -e "${GREEN}✓ All servers started successfully!${NC}"
    echo -e "  Server M (Main) running with PID: $M_PID"
    echo -e "  Server A (Auth) running with PID: $A_PID"
    echo -e "  Server P (Portfolio) running with PID: $P_PID"
    echo -e "  Server Q (Quote) running with PID: $Q_PID"
else
    echo -e "${RED}Error: One or more servers failed to start!${NC}"
    echo -e "\n${YELLOW}Server M log:${NC}"
    cat serverM.log
    echo -e "\n${YELLOW}Server A log:${NC}"
    cat serverA.log
    echo -e "\n${YELLOW}Server P log:${NC}"
    cat serverP.log
    echo -e "\n${YELLOW}Server Q log:${NC}"
    cat serverQ.log
    
    # Clean up
    pkill -9 -f "server[MAPQ]" 2>/dev/null || true
    exit 1
fi

# Step 6: Run a quick authentication test
echo -e "\n${YELLOW}Step 6: Running a quick authentication test...${NC}"
echo -e "user1\nsdvv789\nexit" > auth_only.txt
./client < auth_only.txt > auth_only_output.txt

if grep -q "You have been granted access" auth_only_output.txt; then
    echo -e "${GREEN}✓ Authentication test passed!${NC}"
    cat auth_only_output.txt | grep -A 5 "You have been granted access"
else
    echo -e "${RED}✗ Authentication test failed!${NC}"
    cat auth_only_output.txt
    
    echo -e "\n${YELLOW}Server logs:${NC}"
    echo -e "\nServer M log:"
    cat serverM.log
    echo -e "\nServer A log:"
    cat serverA.log
fi

# Step 7: Display instructions for manual testing
echo -e "\n${YELLOW}Step 7: Everything is set up for manual testing${NC}"
echo -e "${GREEN}The Stock Trading System is now running!${NC}"
echo -e "\n${BLUE}Available commands after login:${NC}"
echo -e "  ${GREEN}quote${NC}           - Get quotes for all stocks"
echo -e "  ${GREEN}quote S1${NC}        - Get quote for stock S1"
echo -e "  ${GREEN}position${NC}        - View your portfolio"
echo -e "  ${GREEN}buy S1 10${NC}       - Buy 10 shares of S1"
echo -e "  ${GREEN}sell S1 5${NC}       - Sell 5 shares of S1"
echo -e "  ${GREEN}exit${NC}            - Exit the program"

echo -e "\n${BLUE}Test account:${NC}"
echo -e "  Username: ${GREEN}user1${NC}"
echo -e "  Password: ${GREEN}sdvv789${NC}"

echo -e "\n${BLUE}To use the client, open a new terminal and run:${NC}"
echo -e "  ${GREEN}./client${NC}"

echo -e "\n${YELLOW}Servers will continue running until you stop this script (Ctrl+C)${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop all servers when done testing${NC}"

# Keep script running to maintain servers
while true; do
    sleep 10
done