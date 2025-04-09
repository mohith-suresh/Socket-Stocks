#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System Automated Client Test ====${NC}"
echo -e "${YELLOW}Testing client login with preset credentials${NC}"

# Ensure all servers are running (assumes start_servers.sh is already running)
echo -e "${YELLOW}Note: This script assumes all servers are already running${NC}"
echo -e "${YELLOW}If servers are not running, please run ./start_servers.sh first${NC}"

# Create input file
echo -e "${YELLOW}Creating test input...${NC}"
cat > test_client_input.txt << 'EOL'
user1
sdvv789
QUERY AAPL
QUERY MSFT
TRADES
EXIT
EOL

# Run the client with the input file
echo -e "\n${YELLOW}Running automated client test...${NC}"
echo -e "This will connect to Main Server, authenticate, and run several commands"
echo -e "${YELLOW}Running: ./client < test_client_input.txt > test_client_output.txt${NC}"

# Use timeout to prevent hanging
timeout 15 ./client < test_client_input.txt > test_client_output.txt

# Check if client ran successfully
if [ $? -eq 124 ]; then
    echo -e "${RED}Client test timed out!${NC}"
    echo -e "Check server and client logs for errors."
else
    echo -e "${GREEN}Client test completed!${NC}"
    echo -e "${YELLOW}Output from client:${NC}"
    cat test_client_output.txt
    
    # Check for successful authentication
    if grep -q "Server M] Authentication successful" serverM.log; then
        echo -e "\n${GREEN}Authentication test PASSED!${NC}"
        echo -e "Verified from serverM.log that the server authenticated the client successfully."
    else
        echo -e "\n${RED}Authentication test FAILED!${NC}"
        echo -e "Server did not report successful authentication. Check logs for details."
    fi
fi

echo -e "\n${YELLOW}Test complete.${NC}"