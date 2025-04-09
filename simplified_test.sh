#!/bin/bash

# Ultra simplified test for basic server functionality
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Ultra Simplified Server Test ===${NC}"

# Kill any existing servers
echo -e "${YELLOW}Cleaning up any previous server processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Force rebuild all servers
echo -e "${YELLOW}Rebuilding servers for this system...${NC}"
make clean
make all

# Verify executables
echo -e "${YELLOW}Verifying executables exist...${NC}"
if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ]; then
    echo -e "${RED}Error: Some executables are missing!${NC}"
    ls -la ./server*
    exit 1
fi
echo -e "${GREEN}All executables are present!${NC}"

# Start all servers
echo -e "${YELLOW}Starting Server M...${NC}"
./serverM > serverM.log 2>&1 &
M_PID=$!
sleep 2

echo -e "${YELLOW}Starting Server A...${NC}"
./serverA > serverA.log 2>&1 &
A_PID=$!
sleep 1

echo -e "${YELLOW}Starting Server P...${NC}"
./serverP > serverP.log 2>&1 &
P_PID=$!
sleep 1

echo -e "${YELLOW}Starting Server Q...${NC}"
./serverQ > serverQ.log 2>&1 &
Q_PID=$!
sleep 1

# Check if servers are running
echo -e "\n${YELLOW}Checking server status:${NC}"

if ps -p $M_PID > /dev/null; then
    echo -e "${GREEN}Server M started successfully (PID: $M_PID)!${NC}"
else
    echo -e "${RED}Error: Server M failed to start!${NC}"
    cat serverM.log
fi

if ps -p $A_PID > /dev/null; then
    echo -e "${GREEN}Server A started successfully (PID: $A_PID)!${NC}"
else
    echo -e "${RED}Error: Server A failed to start!${NC}"
    cat serverA.log
fi

if ps -p $P_PID > /dev/null; then
    echo -e "${GREEN}Server P started successfully (PID: $P_PID)!${NC}"
else
    echo -e "${RED}Error: Server P failed to start!${NC}"
    cat serverP.log
fi

if ps -p $Q_PID > /dev/null; then
    echo -e "${GREEN}Server Q started successfully (PID: $Q_PID)!${NC}"
else
    echo -e "${RED}Error: Server Q failed to start!${NC}"
    cat serverQ.log
fi

# Display relevant log information
echo -e "\n${YELLOW}Server M log:${NC}"
cat serverM.log

# Clean up
echo -e "\n${YELLOW}Cleaning up server processes...${NC}"
kill -9 $M_PID $A_PID $P_PID $Q_PID 2>/dev/null || true

echo -e "\n${GREEN}Test completed!${NC}"