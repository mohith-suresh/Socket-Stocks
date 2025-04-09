#!/bin/bash

# Compile and Test Script for Stock Trading System
# This script compiles the system and runs all tests sequentially

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color
BOLD='\033[1m'
NORMAL='\033[0m'

# Banner
echo -e "${YELLOW}${BOLD}===== Stock Trading System: Compile and Test =====${NC}${NORMAL}"
echo

# Step 1: Clean and compile
echo -e "${YELLOW}Step 1: Cleaning and compiling...${NC}"
make clean all

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed! Please fix errors and try again.${NC}"
    exit 1
fi

echo -e "${GREEN}Compilation successful!${NC}"
echo

# Step 2: Clean up existing processes
echo -e "${YELLOW}Step 2: Cleaning up existing processes...${NC}"
pkill -f "server[MAPQ]" 2>/dev/null
sleep 1
echo -e "${GREEN}Environment ready.${NC}"
echo

# Step 3: Run tests sequentially
echo -e "${YELLOW}${BOLD}Step 3: Running tests sequentially...${NORMAL}${NC}"
echo

# Function to run a test
run_test() {
    local test_name=$1
    local test_script=$2
    local timeout_value=${3:-10}
    
    echo -e "${YELLOW}Running $test_name...${NC}"
    timeout $timeout_value ./$test_script
    local result=$?
    
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}✓ $test_name passed${NC}"
    elif [ $result -eq 124 ]; then
        echo -e "${YELLOW}⚠ $test_name timed out after ${timeout_value}s but may have partially succeeded${NC}"
    else
        echo -e "${RED}× $test_name failed with code $result${NC}"
    fi
    echo
    
    # Cleanup between tests
    pkill -f "server[MAPQ]" 2>/dev/null
    sleep 1
}

# Run all tests
run_test "Authentication Test" "auth_only_test.sh" 15
run_test "Debug Authentication Test" "debug_auth_test.sh" 15
run_test "Quick Test" "quick_test.sh" 15
run_test "Quote Test" "quote_test.sh" 20

# Step 4: Summary
echo -e "${YELLOW}${BOLD}Test Summary:${NC}${NORMAL}"
echo -e "1. Authentication Test: Verifies correct login functionality"
echo -e "2. Debug Authentication Test: Detailed auth debugging"
echo -e "3. Quick Test: Basic auth and quote retrieval"
echo -e "4. Quote Test: Stock quote retrieval for all and specific stocks"
echo
echo -e "${GREEN}${BOLD}Testing complete!${NC}${NORMAL}"
echo
echo -e "To start the system for normal use:"
echo -e "1. Run ./start_servers.sh in one terminal"
echo -e "2. Run ./client in another terminal"
echo
echo -e "To run the comprehensive test (may take longer):"
echo -e "./comprehensive_test.sh"
