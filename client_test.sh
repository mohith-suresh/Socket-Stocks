#!/bin/bash

# Client test script
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Client Authentication Test ===${NC}"

# Ensure all servers are running
echo -e "${YELLOW}Making sure servers are running...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Force rebuild all executables
echo -e "${YELLOW}Rebuilding executables for compatibility...${NC}"
make clean
make all

# Verify executables are compatible
echo -e "${YELLOW}Verifying executable compatibility...${NC}"
if ! file ./serverM | grep -q "executable"; then
    echo -e "${RED}Error: Executables are not compatible with this system!${NC}"
    file ./serverM ./serverA ./serverP ./serverQ
    exit 1
fi

# Start all servers
./serverM > serverM.log 2>&1 &
sleep 1
./serverA > serverA.log 2>&1 &
sleep 1
./serverP > serverP.log 2>&1 &
sleep 1
./serverQ > serverQ.log 2>&1 &
sleep 1

# Prepare test input
echo -e "${YELLOW}Preparing test input...${NC}"
echo -e "user1\nsdvv789\nexit" > client_input.txt

# Run the client with input
echo -e "${YELLOW}Running client test with authentication...${NC}"
timeout 10 ./client < client_input.txt > client_output.txt

# Display client output
echo -e "\n${GREEN}Client output:${NC}"
cat client_output.txt

# Check for successful authentication
if grep -q "You have been granted access" client_output.txt; then
    echo -e "\n${GREEN}Authentication test PASSED!${NC}"
else
    echo -e "\n${RED}Authentication test FAILED!${NC}"
    
    # Show additional logs
    echo -e "\n${YELLOW}Server M log:${NC}"
    cat serverM.log
    
    echo -e "\n${YELLOW}Server A log:${NC}"
    cat serverA.log
fi

# Clean up
echo -e "\n${YELLOW}Cleaning up processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true

echo -e "\n${GREEN}Test completed!${NC}"