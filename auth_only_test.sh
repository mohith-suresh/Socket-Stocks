#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Authentication Test ====${NC}"

# Create test results directory if it doesn't exist
mkdir -p test_results

# Check if servers are already running
echo -e "${YELLOW}Checking for existing server processes...${NC}"
if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverA" > /dev/null; then
    echo -e "${YELLOW}Starting servers via the start_servers.sh script...${NC}"
    ./start_servers.sh > server_startup.log 2>&1 &
    SERVER_PID=$!
    echo -e "${YELLOW}Waiting for servers to initialize...${NC}"
    for i in {1..5}; do
        echo -n "."
        sleep 1
    done
    echo ""
    
    # Check again if servers are running
    if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverA" > /dev/null; then
        echo -e "${RED}Error: One or more servers failed to start!${NC}"
        cat server_startup.log
        exit 1
    fi
else
    echo -e "${GREEN}Servers are already running. Using existing server processes.${NC}"
fi

# Create log files for test monitoring
touch serverM.log serverA.log
echo -e "${GREEN}All servers are running!${NC}"

# Run Authentication Tests
echo -e "\n${YELLOW}===== Running Authentication Tests =====${NC}"

# Test 1: Valid authentication
echo -e "\n${YELLOW}Test 1: Valid Authentication${NC}"
echo -e "user1\nsdvv789\nexit" > auth_only.txt
timeout 5 ./client < auth_only.txt > auth_only_output.txt
sleep 1
# Check client output for success
if grep -q "You have been granted access" auth_only_output.txt; then
    echo -e "${GREEN}✓ Valid authentication test passed${NC}"
    echo -e "   (Verified from client output)"
    TEST_RESULT=0
else
    echo -e "${RED}✗ Valid authentication test failed${NC}"
    echo -e "   (Authentication success not found in client output)"
    TEST_RESULT=1
fi

# Print client output
echo -e "\n${YELLOW}Client Output:${NC}"
cat auth_only_output.txt

# Keep servers running for further tests
echo -e "\n${YELLOW}Authentication test completed with result: ${TEST_RESULT}${NC}"
exit $TEST_RESULT