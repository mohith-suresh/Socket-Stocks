#!/bin/bash

# Compile and Test Script for Stock Trading System
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}    Compile and Test Everything    ${NC}"
echo -e "${BLUE}========================================${NC}"

# Step 1: Clean up any previous processes and binaries
echo -e "\n${YELLOW}Step 1: Cleaning up...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
make clean
sleep 1

# Step 2: Rebuild all executables
echo -e "\n${YELLOW}Step 2: Compiling all components...${NC}"
make all

# Step 3: Verify executables
echo -e "\n${YELLOW}Step 3: Verifying executables...${NC}"
if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ] || [ ! -f ./client ]; then
    echo -e "${RED}Error: Compilation failed, some executables are missing!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ All executables compiled successfully${NC}"
echo -e "\n${YELLOW}Executable details:${NC}"
file ./serverM ./serverA ./serverP ./serverQ ./client

# Step 4: Run simplified authentication test
echo -e "\n${YELLOW}Step 4: Running authentication test...${NC}"
./auth_test.sh

# Step 5: Ask if user wants to run comprehensive test
echo -e "\n${YELLOW}Step 5: What would you like to do next?${NC}"
echo -e "1) Run the comprehensive test (all features)"
echo -e "2) Start the system for manual testing"
echo -e "3) Exit"
read -p "Enter your choice (1-3): " choice

case $choice in
    1)
        echo -e "\n${YELLOW}Running comprehensive test...${NC}"
        ./comprehensive_test.sh
        ;;
    2)
        echo -e "\n${YELLOW}Starting the system for manual testing...${NC}"
        ./quick_start.sh
        ;;
    3)
        echo -e "\n${GREEN}Exiting...${NC}"
        exit 0
        ;;
    *)
        echo -e "\n${RED}Invalid choice. Exiting...${NC}"
        exit 1
        ;;
esac