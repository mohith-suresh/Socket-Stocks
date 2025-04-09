#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Server Startup ====${NC}"

# Kill any existing servers
echo -e "${YELLOW}Cleaning up any previous server processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Force rebuild of servers to ensure they're compatible with this system
echo -e "${YELLOW}Rebuilding all servers for this system...${NC}"
make clean
make all

# Check if binaries were properly built for this system
echo -e "${YELLOW}Verifying executable compatibility...${NC}"
if ! file ./serverM | grep -q "executable" || ! file ./serverA | grep -q "executable"; then
    echo -e "${RED}Error: Executables are not compatible with this system!${NC}"
    echo -e "${YELLOW}Executables details:${NC}"
    file ./serverM ./serverA ./serverP ./serverQ
    exit 1
fi

# Start all servers
echo -e "${YELLOW}Starting Server M (Main Server)...${NC}"
./serverM &
M_PID=$!
sleep 1

echo -e "${YELLOW}Starting Server A (Authentication Server)...${NC}"
./serverA &
A_PID=$!
sleep 1

echo -e "${YELLOW}Starting Server P (Portfolio Server)...${NC}"
./serverP &
P_PID=$!
sleep 1

echo -e "${YELLOW}Starting Server Q (Quote Server)...${NC}"
./serverQ &
Q_PID=$!
sleep 1

# Verify servers are running
if ps -p $M_PID > /dev/null && ps -p $A_PID > /dev/null && ps -p $P_PID > /dev/null && ps -p $Q_PID > /dev/null; then
    echo -e "${GREEN}All servers started successfully!${NC}"
    echo -e "Server M PID: $M_PID"
    echo -e "Server A PID: $A_PID"
    echo -e "Server P PID: $P_PID"
    echo -e "Server Q PID: $Q_PID"
else
    echo -e "${RED}Error: One or more servers failed to start${NC}"
    exit 1
fi

# Display server port information for reference
echo -e "\n${YELLOW}Server Port Information:${NC}"
echo -e "Server M: TCP port 45000, UDP port 44000"
echo -e "Server A: UDP port 41000"
echo -e "Server P: UDP port 42000"
echo -e "Server Q: UDP port 43000"

echo -e "\n${YELLOW}To test the client, open another terminal and run:${NC}"
echo -e "./client\n"
echo -e "${YELLOW}Test account:${NC}"
echo -e "Username: user1"
echo -e "Password: sdvv789"

# Keep the script running to keep servers alive
echo -e "\n${YELLOW}Servers are now running. Press Ctrl+C to stop all servers...${NC}"
while true; do
    sleep 10
done