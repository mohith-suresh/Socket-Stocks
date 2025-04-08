#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to cleanup all processes on exit
cleanup() {
    echo -e "${YELLOW}Cleaning up server processes...${NC}"
    pkill -f "./serverM" || true
    pkill -f "./serverA" || true
    pkill -f "./serverP" || true
    pkill -f "./serverQ" || true
    echo -e "${GREEN}Cleanup complete${NC}"
    exit $1
}

# Set up trap to catch Ctrl+C
trap 'cleanup 1' INT

# Show menu
show_menu() {
    echo -e "${BLUE}======================================================${NC}"
    echo -e "${BLUE}         Stock Trading System Test Suite              ${NC}"
    echo -e "${BLUE}======================================================${NC}"
    echo -e "${YELLOW}1. Run Automatic Test (without expect)${NC}"
    echo -e "${YELLOW}2. Start Servers for Manual Testing${NC}"
    echo -e "${YELLOW}3. Check Server Logs${NC}"
    echo -e "${YELLOW}4. Compile All Programs${NC}"
    echo -e "${YELLOW}5. Exit${NC}"
    echo -e "${BLUE}======================================================${NC}"
    echo -ne "${GREEN}Please select an option [1-5]: ${NC}"
}

# Check if executables exist
check_executables() {
    if [ ! -f ./serverM ] || [ ! -f ./serverA ] || [ ! -f ./serverP ] || [ ! -f ./serverQ ] || [ ! -f ./client ]; then
        echo -e "${RED}Error: Missing executable files. Make sure to run 'make all' first.${NC}"
        return 1
    fi
    return 0
}

# Start all servers
start_servers() {
    # Check if input files exist
    if [ ! -f ./members.txt ]; then
        echo -e "${RED}Error: members.txt file is missing${NC}"
        return 1
    fi

    if [ ! -f ./portfolios.txt ]; then
        echo -e "${RED}Error: portfolios.txt file is missing${NC}"
        return 1
    fi

    if [ ! -f ./quotes.txt ]; then
        echo -e "${RED}Error: quotes.txt file is missing${NC}"
        return 1
    fi

    # Check if servers are already running
    if pgrep -f "./serverM" > /dev/null || pgrep -f "./serverA" > /dev/null || \
       pgrep -f "./serverP" > /dev/null || pgrep -f "./serverQ" > /dev/null; then
        echo -e "${RED}Error: Some servers are already running. Please stop them first.${NC}"
        return 1
    fi

    # Start servers in the correct order
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

    # Verify servers are running
    if ! pgrep -f "./serverM" > /dev/null; then
        echo -e "${RED}Error: Server M failed to start. Check serverM.log for details.${NC}"
        return 1
    fi

    if ! pgrep -f "./serverA" > /dev/null; then
        echo -e "${RED}Error: Server A failed to start. Check serverA.log for details.${NC}"
        return 1
    fi

    if ! pgrep -f "./serverP" > /dev/null; then
        echo -e "${RED}Error: Server P failed to start. Check serverP.log for details.${NC}"
        return 1
    fi

    if ! pgrep -f "./serverQ" > /dev/null; then
        echo -e "${RED}Error: Server Q failed to start. Check serverQ.log for details.${NC}"
        return 1
    fi

    echo -e "${GREEN}All servers started successfully${NC}"
    return 0
}

# Run automatic test
run_auto_test() {
    check_executables || return 1
    start_servers || return 1

    # Create a temporary file for commands
    cat > commands.txt << EOF
user1
sdvv789
quote
quote S1
buy S1 5
yes
position
sell S1 2
yes
exit
EOF

    # Run client with the commands file
    echo -e "${YELLOW}Running automated client test...${NC}"
    ./client < commands.txt > client_output.txt 2>&1

    # Check results
    echo -e "${BLUE}======================================================${NC}"
    echo -e "${BLUE}             Automated Test Results                   ${NC}"
    echo -e "${BLUE}======================================================${NC}"

    if grep -q "granted access" client_output.txt; then
        echo -e "${GREEN}✓ Login successful${NC}"
    else
        echo -e "${RED}✗ Login failed${NC}"
    fi

    if grep -q "S1 " client_output.txt; then
        echo -e "${GREEN}✓ Quote command successful${NC}"
    else
        echo -e "${RED}✗ Quote command failed${NC}"
    fi

    if grep -q "BUY CONFIRMED" client_output.txt; then
        echo -e "${GREEN}✓ Buy command successful${NC}"
    else
        echo -e "${YELLOW}? Buy command may have failed - check the output${NC}"
    fi

    if grep -q "portfolio" client_output.txt; then
        echo -e "${GREEN}✓ Position command successful${NC}"
    else
        echo -e "${RED}✗ Position command failed${NC}"
    fi

    if grep -q "SELL CONFIRMED" client_output.txt; then
        echo -e "${GREEN}✓ Sell command successful${NC}"
    else
        echo -e "${YELLOW}? Sell command may have failed - check the output${NC}"
    fi

    echo -e "${BLUE}======================================================${NC}"
    echo -e "${YELLOW}Would you like to see the complete client output? (y/n): ${NC}"
    read -r show_output
    if [[ "$show_output" == "y" || "$show_output" == "Y" ]]; then
        echo -e "${YELLOW}Complete client output:${NC}"
        cat client_output.txt
    fi

    # Cleanup
    rm -f commands.txt
    return 0
}

# Start servers for manual testing
run_manual_test() {
    check_executables || return 1
    start_servers || return 1
    
    # Display running processes
    echo -e "${YELLOW}Checking running server processes:${NC}"
    ps aux | grep -E "server[MAPQ]" | grep -v grep

    # Manual testing instructions
    echo -e "${GREEN}===================================================${NC}"
    echo -e "${GREEN}All servers are running. Now you can test manually.${NC}"
    echo -e "${GREEN}Use the following commands in another terminal:${NC}"
    echo -e "${YELLOW}./client${NC}"
    echo -e "${GREEN}===================================================${NC}"
    echo -e "${YELLOW}Test commands:${NC}"
    echo -e "1. Login with username: ${GREEN}user1${NC} password: ${GREEN}sdvv789${NC}"
    echo -e "2. Get all quotes: ${GREEN}quote${NC}"
    echo -e "3. Get specific quote: ${GREEN}quote S1${NC}"
    echo -e "4. Buy shares: ${GREEN}buy S1 5${NC} (confirm with ${GREEN}yes${NC})"
    echo -e "5. Check position: ${GREEN}position${NC}"
    echo -e "6. Sell shares: ${GREEN}sell S1 2${NC} (confirm with ${GREEN}yes${NC})"
    echo -e "7. Exit: ${GREEN}exit${NC}"
    echo -e "${GREEN}===================================================${NC}"
    echo -e "${YELLOW}Press Enter to stop the servers, or Ctrl+C to exit...${NC}"
    read -r
    return 0
}

# Check server logs
check_logs() {
    echo -e "${BLUE}======================================================${NC}"
    echo -e "${BLUE}               Server Logs                            ${NC}"
    echo -e "${BLUE}======================================================${NC}"
    
    echo -e "${YELLOW}1. Server M Log${NC}"
    echo -e "${YELLOW}2. Server A Log${NC}"
    echo -e "${YELLOW}3. Server P Log${NC}"
    echo -e "${YELLOW}4. Server Q Log${NC}"
    echo -e "${YELLOW}5. Client Output Log${NC}"
    echo -e "${YELLOW}6. Return to Main Menu${NC}"
    
    echo -ne "${GREEN}Please select a log to view [1-6]: ${NC}"
    read -r log_choice
    
    case $log_choice in
        1)
            if [ -f serverM.log ]; then
                echo -e "${YELLOW}Server M Log:${NC}"
                less serverM.log || cat serverM.log
            else
                echo -e "${RED}Server M log file not found${NC}"
            fi
            ;;
        2)
            if [ -f serverA.log ]; then
                echo -e "${YELLOW}Server A Log:${NC}"
                less serverA.log || cat serverA.log
            else
                echo -e "${RED}Server A log file not found${NC}"
            fi
            ;;
        3)
            if [ -f serverP.log ]; then
                echo -e "${YELLOW}Server P Log:${NC}"
                less serverP.log || cat serverP.log
            else
                echo -e "${RED}Server P log file not found${NC}"
            fi
            ;;
        4)
            if [ -f serverQ.log ]; then
                echo -e "${YELLOW}Server Q Log:${NC}"
                less serverQ.log || cat serverQ.log
            else
                echo -e "${RED}Server Q log file not found${NC}"
            fi
            ;;
        5)
            if [ -f client_output.txt ]; then
                echo -e "${YELLOW}Client Output Log:${NC}"
                less client_output.txt || cat client_output.txt
            else
                echo -e "${RED}Client output log file not found${NC}"
            fi
            ;;
        6)
            return 0
            ;;
        *)
            echo -e "${RED}Invalid option${NC}"
            ;;
    esac
    
    return 0
}

# Compile all programs
compile_programs() {
    echo -e "${YELLOW}Compiling all programs...${NC}"
    make clean && make all
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Compilation successful${NC}"
    else
        echo -e "${RED}Compilation failed${NC}"
    fi
    
    return 0
}

# Main loop
while true; do
    show_menu
    read -r choice
    
    case $choice in
        1)
            run_auto_test
            ;;
        2)
            run_manual_test
            ;;
        3)
            check_logs
            ;;
        4)
            compile_programs
            ;;
        5)
            echo -e "${GREEN}Exiting...${NC}"
            cleanup 0
            ;;
        *)
            echo -e "${RED}Invalid option${NC}"
            ;;
    esac
    
    echo
    echo -e "${BLUE}Press Enter to continue...${NC}"
    read -r
done