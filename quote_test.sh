#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Quote Test ====${NC}"

# Create test results directory if it doesn't exist
mkdir -p test_results

# Check if servers are already running
echo -e "${YELLOW}Checking for existing server processes...${NC}"
if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverQ" > /dev/null; then
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
    if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverQ" > /dev/null; then
        echo -e "${RED}Error: One or more servers failed to start!${NC}"
        cat server_startup.log
        exit 1
    fi
else
    echo -e "${GREEN}Servers are already running. Using existing server processes.${NC}"
fi

echo -e "${GREEN}All servers are running!${NC}"

# Run Quote Tests
echo -e "\n${YELLOW}===== Running Quote Tests =====${NC}"

# Test 1: Get All Quotes
echo -e "\n${YELLOW}Test 1: Get All Quotes${NC}"
echo -e "user1\nsdvv789\nquote\nexit" > quote_all.txt
timeout 5 ./client < quote_all.txt > quote_all_output.txt
sleep 1
# Check client output for success
if grep -q "S1" quote_all_output.txt && grep -q "S2" quote_all_output.txt; then
    echo -e "${GREEN}✓ Get all quotes test passed${NC}"
    echo -e "   (Verified from client output)"
    ALL_QUOTES_RESULT=0
else
    echo -e "${RED}✗ Get all quotes test failed${NC}"
    echo -e "   (All quotes not found in client output)"
    ALL_QUOTES_RESULT=1
fi

# Test 2: Get Specific Quote
echo -e "\n${YELLOW}Test 2: Get Specific Quote${NC}"
echo -e "user1\nsdvv789\nquote S1\nexit" > quote_specific.txt
# Use a longer timeout to make sure we get the response
timeout 10 ./client < quote_specific.txt > quote_specific_output.txt
sleep 2
# Check client output for success
if grep -q "S1" quote_specific_output.txt && ! grep -q "S2" quote_specific_output.txt; then
    echo -e "${GREEN}✓ Get specific quote test passed${NC}"
    echo -e "   (Verified from client output)"
    SPECIFIC_QUOTE_RESULT=0
else
    echo -e "${RED}✗ Get specific quote test failed${NC}"
    echo -e "   (Specific quote test failed in client output)"
    SPECIFIC_QUOTE_RESULT=1
fi

# Print client output
echo -e "\n${YELLOW}All Quotes Output:${NC}"
cat quote_all_output.txt
echo -e "\n${YELLOW}Specific Quote Output:${NC}"
cat quote_specific_output.txt

# Keep servers running for further tests
echo -e "\n${YELLOW}Quote tests completed with results: All Quotes=${ALL_QUOTES_RESULT}, Specific Quote=${SPECIFIC_QUOTE_RESULT}${NC}"
if [ $ALL_QUOTES_RESULT -eq 0 ] && [ $SPECIFIC_QUOTE_RESULT -eq 0 ]; then
    exit 0
else
    exit 1
fi