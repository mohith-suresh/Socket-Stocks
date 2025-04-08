#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Start servers in the correct order
echo -e "${YELLOW}Starting Server M...${NC}"
./serverM &
sleep 1

echo -e "${YELLOW}Starting Server A...${NC}"
./serverA &
sleep 1

echo -e "${YELLOW}Starting Server P...${NC}"
./serverP &
sleep 1

echo -e "${YELLOW}Starting Server Q...${NC}"
./serverQ &
sleep 1

# Check if servers are running
if ! pgrep -f "./serverM" > /dev/null; then
    echo -e "${RED}Error: Server M failed to start.${NC}"
    cleanup 1
fi

if ! pgrep -f "./serverA" > /dev/null; then
    echo -e "${RED}Error: Server A failed to start.${NC}"
    cleanup 1
fi

if ! pgrep -f "./serverP" > /dev/null; then
    echo -e "${RED}Error: Server P failed to start.${NC}"
    cleanup 1
fi

if ! pgrep -f "./serverQ" > /dev/null; then
    echo -e "${RED}Error: Server Q failed to start.${NC}"
    cleanup 1
fi

echo -e "${GREEN}All servers started successfully${NC}"

# Display running processes
echo -e "${YELLOW}Checking running server processes:${NC}"
ps aux | grep -E "server[MAPQ]" | grep -v grep

# Display test instructions
echo -e "${GREEN}====================================================${NC}"
echo -e "${BLUE}All servers are running. Now you can test manually.${NC}"
echo -e "${BLUE}Use the following commands:${NC}"
echo -e "${GREEN}./client${NC}"
echo -e "${GREEN}====================================================${NC}"
echo -e "${YELLOW}Test commands:${NC}"
echo -e "${BLUE}1. Login with username: ${GREEN}user1${BLUE} password: ${GREEN}sdvv789${NC}"
echo -e "${BLUE}2. Get all quotes: ${GREEN}quote${NC}"
echo -e "${BLUE}3. Get specific quote: ${GREEN}quote S1${NC}"
echo -e "${BLUE}4. Buy shares: ${GREEN}buy S1 5${BLUE} (confirm with ${GREEN}yes${BLUE})${NC}"
echo -e "${BLUE}5. Check position: ${GREEN}position${NC}"
echo -e "${BLUE}6. Sell shares: ${GREEN}sell S1 2${BLUE} (confirm with ${GREEN}yes${BLUE})${NC}"
echo -e "${BLUE}7. Exit: ${GREEN}exit${NC}"
echo -e "${GREEN}====================================================${NC}"

# Keep servers running until user decides to end
read -p "Press Enter to stop the servers..."

# Cleanup when done
cleanup 0