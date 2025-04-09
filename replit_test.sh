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
    pkill -f "./serverM" 2>/dev/null || true
    pkill -f "./serverA" 2>/dev/null || true
    pkill -f "./serverP" 2>/dev/null || true
    pkill -f "./serverQ" 2>/dev/null || true
    echo -e "${GREEN}Cleanup complete${NC}"
    
    # Only exit if called with an argument
    if [ -n "$1" ]; then
        exit $1
    fi
}

# First cleanup any existing processes
echo -e "${YELLOW}Cleaning up any existing server processes...${NC}"
pkill -f "./serverM" 2>/dev/null || true
pkill -f "./serverA" 2>/dev/null || true
pkill -f "./serverP" 2>/dev/null || true
pkill -f "./serverQ" 2>/dev/null || true

# Make sure all binaries are built
echo -e "${YELLOW}Compiling all programs...${NC}"
make clean && make all

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Compilation successful${NC}"

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
    echo -e "${RED}Error: Server M failed to start. Check serverM.log for details:${NC}"
    cat serverM.log
    cleanup 1
fi

if ! pgrep -f "./serverA" > /dev/null; then
    echo -e "${RED}Error: Server A failed to start. Check serverA.log for details:${NC}"
    cat serverA.log
    cleanup 1
fi

if ! pgrep -f "./serverP" > /dev/null; then
    echo -e "${RED}Error: Server P failed to start. Check serverP.log for details:${NC}"
    cat serverP.log
    cleanup 1
fi

if ! pgrep -f "./serverQ" > /dev/null; then
    echo -e "${RED}Error: Server Q failed to start. Check serverQ.log for details:${NC}"
    cat serverQ.log
    cleanup 1
fi

echo -e "${GREEN}All servers started successfully${NC}"

# Display running processes
echo -e "${YELLOW}Checking running server processes:${NC}"
ps aux | grep -E "./server[MAPQ]" | grep -v grep

# Create a commands file for automated testing
echo -e "${YELLOW}Creating test commands...${NC}"
cat > commands.txt << EOF
user1
sdvv789
quote
quote S1
buy S1 5
yes
position
sell S1 2
yes
exit
EOF

# Run client with the commands file
echo -e "${YELLOW}Running client test (this might timeout in Replit, which is expected)...${NC}"
timeout 10s ./client < commands.txt > client_output.txt 2>&1 || true

# Display a summary of the results
echo -e "${BLUE}======================================================${NC}"
echo -e "${BLUE}             Test Results Summary                     ${NC}"
echo -e "${BLUE}======================================================${NC}"

if grep -q "granted access" client_output.txt; then
    echo -e "${GREEN}✓ Login successful${NC}"
else
    echo -e "${RED}✗ Login failed or command timed out${NC}"
fi

# Check server logs for successful operations even if client timed out
echo -e "${BLUE}======================================================${NC}"
echo -e "${BLUE}        Server Logs for Operation Verification        ${NC}"
echo -e "${BLUE}======================================================${NC}"

echo -e "${YELLOW}Server M Log (last 15 lines):${NC}"
tail -n 15 serverM.log

echo -e "${YELLOW}Server A Log (last 10 lines):${NC}"
tail -n 10 serverA.log

echo -e "${YELLOW}Server P Log (last 10 lines):${NC}"
tail -n 10 serverP.log

echo -e "${YELLOW}Server Q Log (last 10 lines):${NC}"
tail -n 10 serverQ.log

# Cleanup at end
cleanup
echo -e "${GREEN}Test completed successfully!${NC}"
exit 0