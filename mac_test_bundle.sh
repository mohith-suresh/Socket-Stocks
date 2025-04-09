#!/bin/bash

# One-stop setup and testing for MacOS systems
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Mac Setup and Test ====${NC}"

# Step 1: Clean any existing binaries
echo -e "${YELLOW}Step 1: Cleaning existing binaries...${NC}"
make clean

# Step 2: Compile all components
echo -e "${YELLOW}Step 2: Compiling all components...${NC}"
make all

# Check if compilation was successful
if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ] || [ ! -f ./client ]; then
    echo -e "${RED}Compilation failed! Some binaries are missing.${NC}"
    echo -e "Please make sure you have g++ installed and proper C++ development tools."
    exit 1
fi

echo -e "${GREEN}All components compiled successfully!${NC}"

# Step 3: Verify executables
echo -e "${YELLOW}Step 3: Verifying executable files:${NC}"
file ./serverM ./serverA ./serverP ./serverQ ./client

# Step 4: Start servers in background
echo -e "${YELLOW}Step 4: Starting all servers in background...${NC}"
# Kill any existing servers first
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Start servers and save PIDs
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

# Verify servers are running
if ps -p $M_PID > /dev/null && ps -p $A_PID > /dev/null && ps -p $P_PID > /dev/null && ps -p $Q_PID > /dev/null; then
    echo -e "${GREEN}All servers started successfully!${NC}"
    echo -e "Server M (Main) PID: $M_PID"
    echo -e "Server A (Auth) PID: $A_PID"
    echo -e "Server P (Portfolio) PID: $P_PID"
    echo -e "Server Q (Quote) PID: $Q_PID"
else
    echo -e "${RED}Error: One or more servers failed to start.${NC}"
    echo -e "Please check the server log files for more information."
    echo -e "Server M log:"
    cat serverM.log
    pkill -9 -f "server[MAPQ]" 2>/dev/null || true
    exit 1
fi

# Step 5: Run manual client test
echo -e "\n${YELLOW}Step 5: Running client test...${NC}"
echo -e "${YELLOW}Test account:${NC}"
echo -e "Username: user1"
echo -e "Password: sdvv789"
echo -e "\n${GREEN}Press Enter to start the client (or Ctrl+C to exit)...${NC}"
read

# Start client
./client

# Cleanup servers when client exits
echo -e "\n${YELLOW}Cleaning up server processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true

echo -e "\n${GREEN}Test completed!${NC}"
echo -e "If you want to run the servers again, use: ./start_servers.sh"
echo -e "Then connect with the client using: ./client"