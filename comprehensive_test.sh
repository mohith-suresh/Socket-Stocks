#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Comprehensive Test ====${NC}"

# Create test results directory if it doesn't exist
mkdir -p test_results

# Kill any existing servers
echo -e "${YELLOW}Cleaning up any previous server processes...${NC}"
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Force rebuild all executables
echo -e "${YELLOW}Rebuilding all executables...${NC}"
make clean
make all

# Verify the executables are proper for this system
echo -e "${YELLOW}Verifying executables are compatible with this system...${NC}"
if ! file ./serverM | grep -q "executable" || ! file ./serverA | grep -q "executable"; then
    echo -e "${RED}Error: Executables are not compatible with this system!${NC}"
    echo -e "${YELLOW}Executables details:${NC}"
    file ./serverM ./serverA ./serverP ./serverQ ./client
    exit 1
fi

# Start servers with detailed logging
rm -f serverM.log serverA.log serverP.log serverQ.log
echo -e "${YELLOW}Starting servers with detailed logging...${NC}"

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

# Check if all servers are running
if ! pgrep -f "./serverM" > /dev/null || ! pgrep -f "./serverA" > /dev/null || \
   ! pgrep -f "./serverP" > /dev/null || ! pgrep -f "./serverQ" > /dev/null; then
    echo -e "${RED}Error: One or more servers failed to start!${NC}"
    echo -e "${YELLOW}Server M log:${NC}"
    cat serverM.log
    echo -e "${YELLOW}Server A log:${NC}"
    cat serverA.log
    echo -e "${YELLOW}Server P log:${NC}"
    cat serverP.log
    echo -e "${YELLOW}Server Q log:${NC}"
    cat serverQ.log
    exit 1
fi

echo -e "${GREEN}All servers started successfully!${NC}"

# Run a series of tests with different client operations
echo -e "\n${YELLOW}===== Running Authentication Tests =====${NC}"

# Test 1: Valid authentication
echo -e "\n${YELLOW}Test 1: Valid Authentication${NC}"
echo -e "user1\nsdvv789\nexit" > test_input.txt
./client < test_input.txt > test_results/auth_valid.log
if grep -q "You have been granted access" test_results/auth_valid.log; then
    echo -e "${GREEN}✓ Valid authentication test passed${NC}"
else
    echo -e "${RED}✗ Valid authentication test failed${NC}"
fi

# Test 2: Invalid authentication
echo -e "\n${YELLOW}Test 2: Invalid Authentication${NC}"
echo -e "user1\nwrongpass\nexit" > test_input.txt
./client < test_input.txt > test_results/auth_invalid.log
if grep -q "credentials are incorrect" test_results/auth_invalid.log; then
    echo -e "${GREEN}✓ Invalid authentication test passed${NC}"
else
    echo -e "${RED}✗ Invalid authentication test failed${NC}"
fi

echo -e "\n${YELLOW}===== Running Quote Tests =====${NC}"

# Test 3: Get All Quotes
echo -e "\n${YELLOW}Test 3: Get All Quotes${NC}"
echo -e "user1\nsdvv789\nquote\nexit" > test_input.txt
./client < test_input.txt > test_results/quote_all.log
if grep -q "S1" test_results/quote_all.log && grep -q "S2" test_results/quote_all.log; then
    echo -e "${GREEN}✓ Get all quotes test passed${NC}"
else
    echo -e "${RED}✗ Get all quotes test failed${NC}"
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
if grep -q "confirm" test_results/buy.log; then
    echo -e "${GREEN}✓ Buy stocks test passed${NC}"
else
    echo -e "${RED}✗ Buy stocks test failed${NC}"
fi

# Test 7: Sell Stocks
echo -e "\n${YELLOW}Test 7: Sell Stocks${NC}"
echo -e "user1\nsdvv789\nsell S1 2\nyes\nexit" > test_input.txt
./client < test_input.txt > test_results/sell.log
if grep -q "confirm" test_results/sell.log; then
    echo -e "${GREEN}✓ Sell stocks test passed${NC}"
else
    echo -e "${RED}✗ Sell stocks test failed${NC}"
fi

# Test 8: Invalid Command
echo -e "\n${YELLOW}Test 8: Invalid Command${NC}"
echo -e "user1\nsdvv789\nfoobar\nexit" > test_input.txt
./client < test_input.txt > test_results/invalid_command.log
if grep -q "ERROR" test_results/invalid_command.log || grep -q "Unknown" test_results/invalid_command.log; then
    echo -e "${GREEN}✓ Invalid command test passed${NC}"
else
    echo -e "${RED}✗ Invalid command test failed${NC}"
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

# Clean up
echo -e "\n${YELLOW}Cleaning up server processes...${NC}"
pkill -9 -f "./serverM" 2>/dev/null || true
pkill -9 -f "./serverA" 2>/dev/null || true
pkill -9 -f "./serverP" 2>/dev/null || true
pkill -9 -f "./serverQ" 2>/dev/null || true

echo -e "\n${GREEN}All tests completed!${NC}"
echo -e "Detailed test results are available in the test_results directory"