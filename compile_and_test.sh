#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Compiling Stock Trading System ====${NC}"

# Clean any existing binaries
echo -e "${YELLOW}Cleaning existing binaries...${NC}"
make clean

# Compile all components
echo -e "${YELLOW}Compiling all components...${NC}"
make all

# Check if compilation was successful
if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ] || [ ! -f ./client ]; then
    echo -e "${RED}Compilation failed! Some binaries are missing.${NC}"
    exit 1
fi

echo -e "${GREEN}All components compiled successfully!${NC}"

# Display file information for verification
echo -e "${YELLOW}Verifying executable files:${NC}"
file ./serverM ./serverA ./serverP ./serverQ ./client

echo -e "\n${GREEN}System is ready for testing!${NC}"
echo -e "${YELLOW}You can now run:${NC}"
echo -e "  ./start_servers.sh  - To start all servers"
echo -e "  ./client            - To connect as a client"
echo -e "  ./manual_auth_test.sh  - For a guided authentication test"
echo -e "  ./quick_test.sh     - For a quick automated test (with servers already running)"