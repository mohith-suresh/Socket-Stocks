#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Quick Test ====${NC}"

# Check if servers are already running
echo -e "${YELLOW}Checking for existing server processes...${NC}"
pgrep -f "server[MAPQ]" > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${YELLOW}Starting servers...${NC}"
    # Start servers in the background
    ./start_servers.sh > /dev/null 2>&1 &
    SERVER_PID=$!
    
    # Wait for servers to start
    echo -e "${YELLOW}Waiting for servers to initialize...${NC}"
    for i in {1..5}; do
        echo -n "."
        sleep 1
    done
    echo ""
fi

# Create test input
echo -e "${YELLOW}Preparing test input...${NC}"
cat > test_input.txt << EOF
user1
sdvv789
quote
quote S1
exit
EOF

# Run quick test
echo -e "${YELLOW}Running quick test...${NC}"
timeout 10 ./client < test_input.txt > test_output.txt

# Check results
echo -e "${YELLOW}Checking results...${NC}"
if grep -q "You have been granted access" test_output.txt; then
    echo -e "${GREEN}✓ Authentication successful${NC}"
    AUTH_RESULT="PASS"
else
    echo -e "${RED}✗ Authentication failed${NC}"
    AUTH_RESULT="FAIL"
fi

if grep -q "S1" test_output.txt && grep -q "S2" test_output.txt; then
    echo -e "${GREEN}✓ All quotes displayed correctly${NC}"
    ALL_QUOTES_RESULT="PASS"
else
    echo -e "${RED}✗ All quotes display failed${NC}"
    ALL_QUOTES_RESULT="FAIL"
fi

# Overall result
echo -e "\n${YELLOW}Test Summary:${NC}"
echo -e "Authentication: ${AUTH_RESULT}"
echo -e "All Quotes: ${ALL_QUOTES_RESULT}"

echo -e "\n${YELLOW}Client Output:${NC}"
cat test_output.txt

echo -e "\n${YELLOW}Test complete!${NC}"