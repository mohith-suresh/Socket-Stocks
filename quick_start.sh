#!/bin/bash

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print header
echo -e "${BLUE}====================================================${NC}"
echo -e "${BLUE}       EE450 Stock Trading System Quick Start       ${NC}"
echo -e "${BLUE}====================================================${NC}"

# Compile all executables if needed
if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ] || [ ! -f ./client ]; then
    echo -e "${YELLOW}Compiling project executables...${NC}"
    make clean && make all
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed! Please fix errors and try again.${NC}"
        exit 1
    fi
    echo -e "${GREEN}Compilation successful!${NC}"
fi

# Display menu for different testing options
echo -e "${YELLOW}Select a testing option:${NC}"
echo -e "1) ${GREEN}Start all servers${NC} (for production/grading use)"
echo -e "2) ${GREEN}Run single client test${NC} (automated test with predefined commands)"
echo -e "3) ${GREEN}Run interactive client${NC} (manual testing)"
echo -e "q) ${RED}Quit${NC}"
read -p "Enter your choice [1-3 or q]: " choice

case $choice in
    1)
        echo -e "${YELLOW}Starting all servers...${NC}"
        ./start_servers.sh
        ;;
    2)
        echo -e "${YELLOW}Running automated client test...${NC}"
        # First make sure no servers are running
        pkill -f "./serverM" 2>/dev/null || true
        pkill -f "./serverA" 2>/dev/null || true
        pkill -f "./serverP" 2>/dev/null || true
        pkill -f "./serverQ" 2>/dev/null || true
        
        # Start servers
        ./serverM > serverM.log 2>&1 &
        sleep 1
        ./serverA > serverA.log 2>&1 &
        sleep 1
        ./serverP > serverP.log 2>&1 &
        sleep 1
        ./serverQ > serverQ.log 2>&1 &
        sleep 1
        
        # Create test commands
        echo -e "${BLUE}Creating test commands file...${NC}"
        cat > test_commands.txt << EOF
user1
sdvv789
quote
quote S1
buy S1 5
yes
position
sell S1 2
yes
position
exit
EOF
        
        # Run client with test commands
        echo -e "${BLUE}Running client with test commands...${NC}"
        ./client < test_commands.txt
        
        # Cleanup
        echo -e "${YELLOW}Test complete. Cleaning up...${NC}"
        pkill -f "./serverM" 2>/dev/null || true
        pkill -f "./serverA" 2>/dev/null || true
        pkill -f "./serverP" 2>/dev/null || true
        pkill -f "./serverQ" 2>/dev/null || true
        ;;
    3)
        echo -e "${YELLOW}Starting servers for interactive client testing...${NC}"
        # First make sure no servers are running
        pkill -f "./serverM" 2>/dev/null || true
        pkill -f "./serverA" 2>/dev/null || true
        pkill -f "./serverP" 2>/dev/null || true
        pkill -f "./serverQ" 2>/dev/null || true
        
        # Start servers
        ./serverM > serverM.log 2>&1 &
        sleep 1
        ./serverA > serverA.log 2>&1 &
        sleep 1
        ./serverP > serverP.log 2>&1 &
        sleep 1
        ./serverQ > serverQ.log 2>&1 &
        sleep 1
        
        echo -e "${GREEN}Servers started successfully. Starting client...${NC}"
        echo -e "${BLUE}-----------------------------------------------------${NC}"
        echo -e "${BLUE}Available commands:${NC}"
        echo -e " - Login with: ${GREEN}user1/sdvv789${NC} or ${GREEN}user2/sdvvzrug${NC} or ${GREEN}admin/vhfuhwsdvv${NC}"
        echo -e " - ${GREEN}quote${NC} - List all stock quotes"
        echo -e " - ${GREEN}quote S1${NC} - Get quote for stock S1"
        echo -e " - ${GREEN}buy S1 5${NC} - Buy 5 shares of S1"
        echo -e " - ${GREEN}sell S1 2${NC} - Sell 2 shares of S1"
        echo -e " - ${GREEN}position${NC} - View your portfolio"
        echo -e " - ${GREEN}exit${NC} - Exit client"
        echo -e "${BLUE}-----------------------------------------------------${NC}"
        
        # Start client
        ./client
        
        # Cleanup
        echo -e "${YELLOW}Session complete. Cleaning up...${NC}"
        pkill -f "./serverM" 2>/dev/null || true
        pkill -f "./serverA" 2>/dev/null || true
        pkill -f "./serverP" 2>/dev/null || true
        pkill -f "./serverQ" 2>/dev/null || true
        ;;
    q|Q)
        echo -e "${RED}Exiting...${NC}"
        exit 0
        ;;
    *)
        echo -e "${RED}Invalid option.${NC}"
        exit 1
        ;;
esac

echo -e "${GREEN}Done!${NC}"
exit 0