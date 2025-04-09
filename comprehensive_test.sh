#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Comprehensive Test ====${NC}"

# Create test results directory if it doesn't exist
mkdir -p test_results

# Check if servers are already running
echo -e "${YELLOW}Checking for existing server processes...${NC}"
if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverA" > /dev/null || \
   ! pgrep -f "./serverP" > /dev/null || ! pgrep -f "./serverQ" > /dev/null; then
    echo -e "${YELLOW}Starting servers via the start_servers.sh script...${NC}"
    ./start_servers.sh > /dev/null 2>&1 &
    SERVER_PID=$!
    echo -e "${YELLOW}Waiting for servers to initialize...${NC}"
    for i in {1..10}; do
        echo -n "."
        sleep 1
    done
    echo ""
    
    # Check again if servers are running
    if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverA" > /dev/null || \
       ! pgrep -f "./serverP" > /dev/null || ! pgrep -f "./serverQ" > /dev/null; then
        echo -e "${RED}Error: One or more servers failed to start!${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}Servers are already running. Using existing server processes.${NC}"
fi

# Create log files for test monitoring
touch serverM.log serverA.log serverP.log serverQ.log
echo -e "${GREEN}All servers are running!${NC}"

# Run a series of tests with different client operations
echo -e "\n${YELLOW}===== Running Authentication Tests =====${NC}"

# Test 1: Valid authentication
echo -e "\n${YELLOW}Test 1: Valid Authentication${NC}"
echo -e "user1\nsdvv789\nexit" > test_input.txt
timeout 10 ./client < test_input.txt > test_results/auth_valid.log
# Give servers time to process
sleep 2
# Check serverM.log for authentication success or client output
if grep -q "Authentication successful for user user1" serverM.log || grep -q "You have been granted access" test_results/auth_valid.log; then
    echo -e "${GREEN}✓ Valid authentication test passed${NC}"
    echo -e "   (Verified from serverM.log or client output)"
else
    echo -e "${RED}✗ Valid authentication test failed${NC}"
    echo -e "   (Authentication success not found in logs or client output)"
fi

# Test 2: Invalid authentication
echo -e "\n${YELLOW}Test 2: Invalid Authentication${NC}"
echo -e "user1\nwrongpass\nexit" > test_input.txt
timeout 10 ./client < test_input.txt > test_results/auth_invalid.log
# Give servers time to process
sleep 2
# Check from server logs or client output
if grep -q "Authentication failed for user" serverM.log || grep -q "credentials are incorrect" test_results/auth_invalid.log; then
    echo -e "${GREEN}✓ Invalid authentication test passed${NC}"
    echo -e "   (Verified from serverM.log or client output)"
else
    echo -e "${RED}✗ Invalid authentication test failed${NC}"
    echo -e "   (Authentication failure not properly detected)"
fi

echo -e "\n${YELLOW}===== Running Quote Tests =====${NC}"

# Test 3: Get All Quotes
echo -e "\n${YELLOW}Test 3: Get All Quotes${NC}"
echo -e "user1\nsdvv789\nquote\nexit" > test_input.txt
timeout 10 ./client < test_input.txt > test_results/quote_all.log
# Give servers time to process
sleep 2
# Check server logs or client output for quote request
if grep -q "Received quote request for all stocks" serverM.log || (grep -q "S1" test_results/quote_all.log && grep -q "S2" test_results/quote_all.log); then
    echo -e "${GREEN}✓ Get all quotes test passed${NC}"
    echo -e "   (Verified from server logs or client output)"
else
    echo -e "${RED}✗ Get all quotes test failed${NC}"
    echo -e "   (Quote request not properly processed)"
fi

# Test 4: Get Specific Quote
echo -e "\n${YELLOW}Test 4: Get Specific Quote${NC}"
echo -e "user1\nsdvv789\nquote S1\nexit" > test_input.txt
./client < test_input.txt > test_results/quote_specific.log
if grep -q "S1" test_results/quote_specific.log && ! grep -q "S2" test_results/quote_specific.log; then
    echo -e "${GREEN}✓ Get specific quote test passed${NC}"
else
    echo -e "${RED}✗ Get specific quote test failed${NC}"
fi

echo -e "\n${YELLOW}===== Running Portfolio Tests =====${NC}"

# Test 5: Get Portfolio
echo -e "\n${YELLOW}Test 5: Get Portfolio${NC}"
echo -e "user1\nsdvv789\nposition\nexit" > test_input.txt
./client < test_input.txt > test_results/portfolio.log
if grep -q "portfolio" test_results/portfolio.log || grep -q "Position" test_results/portfolio.log; then
    echo -e "${GREEN}✓ Get portfolio test passed${NC}"
else
    echo -e "${RED}✗ Get portfolio test failed${NC}"
fi

echo -e "\n${YELLOW}===== Running Transaction Tests =====${NC}"

# Test 6: Buy Stocks
echo -e "\n${YELLOW}Test 6: Buy Stocks${NC}"
echo -e "user1\nsdvv789\nbuy S1 5\nyes\nexit" > test_input.txt
./client < test_input.txt > test_results/buy.log
if grep -qi "confirm" test_results/buy.log || grep -q "BUY CONFIRM" test_results/buy.log; then
    echo -e "${GREEN}✓ Buy stocks test passed${NC}"
else
    echo -e "${RED}✗ Buy stocks test failed${NC}"
fi

# Test 7: Sell Stocks
echo -e "\n${YELLOW}Test 7: Sell Stocks${NC}"
echo -e "user1\nsdvv789\nsell S1 2\nyes\nexit" > test_input.txt
./client < test_input.txt > test_results/sell.log
if grep -qi "confirm" test_results/sell.log || grep -q "SELL CONFIRM" test_results/sell.log; then
    echo -e "${GREEN}✓ Sell stocks test passed${NC}"
else
    echo -e "${RED}✗ Sell stocks test failed${NC}"
fi

# Test 8: Invalid Command
echo -e "\n${YELLOW}Test 8: Invalid Command${NC}"
echo -e "user1\nsdvv789\nfoobar\nexit" > test_input.txt
./client < test_input.txt > test_results/invalid_command.log
if grep -q "ERROR" test_results/invalid_command.log || grep -q "Unknown" test_results/invalid_command.log || grep -q "Invalid" test_results/invalid_command.log || grep -q "unrecognized" test_results/invalid_command.log; then
    echo -e "${GREEN}✓ Invalid command test passed${NC}"
else
    echo -e "${RED}✗ Invalid command test failed${NC}"
    # Show what error message is actually being displayed
    echo -e "   Actual output: "
    grep -A 2 "foobar" test_results/invalid_command.log | head -3
fi

# Review server logs
echo -e "\n${YELLOW}===== Server Logs =====${NC}"

echo -e "\n${YELLOW}Server M log:${NC}"
grep -v "Resource temporarily unavailable" serverM.log | tail -n 20

echo -e "\n${YELLOW}Server A log:${NC}"
grep -v "Resource temporarily unavailable" serverA.log | tail -n 10

echo -e "\n${YELLOW}Server P log:${NC}"
grep -v "Resource temporarily unavailable" serverP.log | tail -n 10

echo -e "\n${YELLOW}Server Q log:${NC}"
grep -v "Resource temporarily unavailable" serverQ.log | tail -n 10

# Note that we're not cleaning up servers to allow the workflow to continue
echo -e "\n${YELLOW}Leaving servers running for further testing...${NC}"

echo -e "\n${GREEN}All tests completed!${NC}"
echo -e "Detailed test results are available in the test_results directory"