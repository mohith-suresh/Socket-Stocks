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

# Create a temporary file for commands
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
echo -e "${YELLOW}Running automated client test...${NC}"
./client < commands.txt > client_output.txt 2>&1

# Check results
if grep -q "granted access" client_output.txt; then
    echo -e "${GREEN}Login successful${NC}"
else
    echo -e "${RED}Login failed${NC}"
    cat client_output.txt
    cleanup 1
fi

if grep -q "S1" client_output.txt; then
    echo -e "${GREEN}Quote command successful${NC}"
else
    echo -e "${RED}Quote command failed${NC}"
    cat client_output.txt
    cleanup 1
fi

if grep -q "BUY CONFIRMED" client_output.txt; then
    echo -e "${GREEN}Buy command successful${NC}"
else
    echo -e "${YELLOW}Buy command may have failed - check the output${NC}"
    grep -A 5 "buy S1" client_output.txt || true
fi

if grep -q "portfolio" client_output.txt; then
    echo -e "${GREEN}Position command successful${NC}"
else
    echo -e "${RED}Position command failed${NC}"
    grep -A 5 "position" client_output.txt || true
fi

if grep -q "SELL CONFIRMED" client_output.txt; then
    echo -e "${GREEN}Sell command successful${NC}"
else
    echo -e "${YELLOW}Sell command may have failed - check the output${NC}"
    grep -A 5 "sell S1" client_output.txt || true
fi

echo -e "${YELLOW}Complete client output:${NC}"
cat client_output.txt

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