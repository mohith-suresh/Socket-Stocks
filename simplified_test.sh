#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to cleanup all processes on exit
cleanup() {
    echo -e "${YELLOW}Cleaning up server processes...${NC}"
    pkill -f "./serverM" || true
    pkill -f "./serverA" || true
    pkill -f "./serverP" || true
    pkill -f "./serverQ" || true
    echo -e "${GREEN}Cleanup complete${NC}"
    exit $1
}

# Set up trap to catch Ctrl+C
trap 'cleanup 1' INT

# Check if executables exist
if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ] || [ ! -f ./client ]; then
    echo -e "${RED}Error: Missing executable files. Make sure to run 'make all' first.${NC}"
    exit 1
fi

# Check if input files exist
if [ ! -f ./members.txt ]; then
    echo -e "${RED}Error: members.txt file is missing${NC}"
    exit 1
fi

if [ ! -f ./portfolios.txt ]; then
    echo -e "${RED}Error: portfolios.txt file is missing${NC}"
    exit 1
fi

if [ ! -f ./quotes.txt ]; then
    echo -e "${RED}Error: quotes.txt file is missing${NC}"
    exit 1
fi

# Start servers in the correct order
echo -e "${YELLOW}Starting Server M...${NC}"
./serverM > serverM.log 2>&1 &
sleep 1

echo -e "${YELLOW}Starting Server A...${NC}"
./serverA > serverA.log 2>&1 &
sleep 1

echo -e "${YELLOW}Starting Server P...${NC}"
./serverP > serverP.log 2>&1 &
sleep 1

echo -e "${YELLOW}Starting Server Q...${NC}"
./serverQ > serverQ.log 2>&1 &
sleep 1

# Check if servers are running
if ! pgrep -f "./serverM" > /dev/null; then
    echo -e "${RED}Error: Server M failed to start. Check serverM.log for details.${NC}"
    cat serverM.log
    cleanup 1
fi

if ! pgrep -f "./serverA" > /dev/null; then
    echo -e "${RED}Error: Server A failed to start. Check serverA.log for details.${NC}"
    cat serverA.log
    cleanup 1
fi

if ! pgrep -f "./serverP" > /dev/null; then
    echo -e "${RED}Error: Server P failed to start. Check serverP.log for details.${NC}"
    cat serverP.log
    cleanup 1
fi

if ! pgrep -f "./serverQ" > /dev/null; then
    echo -e "${RED}Error: Server Q failed to start. Check serverQ.log for details.${NC}"
    cat serverQ.log
    cleanup 1
fi

echo -e "${GREEN}All servers started successfully${NC}"

# Display running processes
echo -e "${YELLOW}Checking running server processes:${NC}"
ps aux | grep -E "server[MAPQ]" | grep -v grep

# Manual testing instructions
echo -e "${GREEN}===================================================${NC}"
echo -e "${GREEN}All servers are running. Now you can test manually.${NC}"
echo -e "${GREEN}Use the following commands in another terminal:${NC}"
echo -e "${YELLOW}./client${NC}"
echo -e "${GREEN}===================================================${NC}"
echo -e "${YELLOW}Test commands:${NC}"
echo -e "1. Login with username: ${GREEN}user1${NC} password: ${GREEN}sdvv789${NC}"
echo -e "2. Get all quotes: ${GREEN}quote${NC}"
echo -e "3. Get specific quote: ${GREEN}quote S1${NC}"
echo -e "4. Buy shares: ${GREEN}buy S1 5${NC} (confirm with ${GREEN}yes${NC})"
echo -e "5. Check position: ${GREEN}position${NC}"
echo -e "6. Sell shares: ${GREEN}sell S1 2${NC} (confirm with ${GREEN}yes${NC})"
echo -e "7. Exit: ${GREEN}exit${NC}"
echo -e "${GREEN}===================================================${NC}"
echo -e "${YELLOW}Press Enter to stop the servers and view logs, or Ctrl+C to exit...${NC}"
read -r

# Display logs for debugging
echo -e "${YELLOW}Server M Log:${NC}"
tail -n 20 serverM.log

echo -e "${YELLOW}Server A Log:${NC}"
tail -n 20 serverA.log

echo -e "${YELLOW}Server P Log:${NC}"
tail -n 20 serverP.log

echo -e "${YELLOW}Server Q Log:${NC}"
tail -n 20 serverQ.log

# Cleanup
cleanup 0