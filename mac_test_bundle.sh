#!/bin/bash

# Mac Testing Bundle for Stock Trading System
# This script provides compatibility and testing tools for macOS users

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color
BOLD='\033[1m'
NORMAL='\033[0m'

# Banner
echo -e "${YELLOW}${BOLD}===== Stock Trading System Mac Test Bundle =====${NC}${NORMAL}"
echo

# Step 1: Check for Mac
echo -e "${YELLOW}Step 1: Checking environment...${NC}"
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${RED}This script is intended for macOS users only.${NC}"
    exit 1
fi

echo -e "${GREEN}Running on macOS: $(sw_vers -productVersion)${NC}"
echo

# Step 2: Check for required tools
echo -e "${YELLOW}Step 2: Checking required tools...${NC}"
for cmd in g++ make grep pkill; do
    if ! command -v $cmd > /dev/null; then
        echo -e "${RED}Required command '$cmd' not found!${NC}"
        if [ "$cmd" == "g++" ]; then
            echo -e "${YELLOW}Please install XCode Command Line Tools with: xcode-select --install${NC}"
        fi
        exit 1
    fi
done

echo -e "${GREEN}All required tools found.${NC}"
echo

# Step 3: Compile
echo -e "${YELLOW}Step 3: Compiling with macOS compatibility...${NC}"

# Check for any macOS-specific compilation flags
if [ -f "Makefile" ]; then
    # Backup original Makefile
    cp Makefile Makefile.bak
    
    # Adjust Makefile if needed for macOS
    if grep -q "std=c++11" Makefile; then
        echo "C++11 flag already present, no changes needed."
    else
        echo "Adding C++11 flag for macOS compatibility."
        sed -i '' 's/CFLAGS=/CFLAGS= -std=c++11 /g' Makefile
    fi
fi

make clean all

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed! Please fix errors and try again.${NC}"
    # Restore original Makefile
    if [ -f "Makefile.bak" ]; then
        mv Makefile.bak Makefile
    fi
    exit 1
fi

echo -e "${GREEN}Compilation successful!${NC}"
echo

# Step 4: Check for existing server processes
echo -e "${YELLOW}Step 4: Checking for existing servers...${NC}"

pgrep -f "serverM" > /dev/null
if [ $? -eq 0 ]; then
    echo -e "${YELLOW}Existing server processes found. Cleaning up...${NC}"
    pkill -f "server[MAPQ]"
    sleep 1
fi

echo -e "${GREEN}Server environment ready.${NC}"
echo

# Step 5: Create File Checker
echo -e "${YELLOW}Step 5: Creating file integrity checker...${NC}"

cat > check_files.sh << 'EOF_INNER'
#!/bin/bash

# File integrity checker for Stock Trading System

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Required files
required_executables=("client" "serverM" "serverA" "serverP" "serverQ")
required_data=("members.txt" "portfolios.txt" "quotes.txt")
required_scripts=("start_servers.sh")

# Check executables
echo -e "${YELLOW}Checking executables...${NC}"
for exe in "${required_executables[@]}"; do
    if [ ! -f "$exe" ] || [ ! -x "$exe" ]; then
        echo -e "${RED}$exe not found or not executable!${NC}"
        echo "Run 'make clean all' to rebuild."
        exit 1
    else
        echo -e "${GREEN}✓ $exe${NC}"
    fi
done

# Check data files
echo -e "\n${YELLOW}Checking data files...${NC}"
for data in "${required_data[@]}"; do
    if [ ! -f "$data" ]; then
        echo -e "${RED}$data not found!${NC}"
        exit 1
    else
        echo -e "${GREEN}✓ $data${NC}"
        echo "First 3 lines:"
        head -n 3 "$data"
        echo
    fi
done

# Check scripts
echo -e "\n${YELLOW}Checking scripts...${NC}"
for script in "${required_scripts[@]}"; do
    if [ ! -f "$script" ] || [ ! -x "$script" ]; then
        echo -e "${RED}$script not found or not executable!${NC}"
        echo "Run 'chmod +x $script' to make it executable."
        exit 1
    else
        echo -e "${GREEN}✓ $script${NC}"
    fi
done

echo -e "\n${GREEN}All required files are present and valid.${NC}"
EOF_INNER

chmod +x check_files.sh
echo -e "${GREEN}File checker created: check_files.sh${NC}"
echo

# Step 6: Create Simplified Test Script
echo -e "${YELLOW}Step 6: Creating simplified test script...${NC}"

cat > simplified_test.sh << 'EOF_INNER'
#!/bin/bash

# Simplified Test Script for Stock Trading System on macOS

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Create test directory
mkdir -p mac_test_results

echo -e "${YELLOW}Starting servers...${NC}"
# Start servers in background
./start_servers.sh > mac_test_results/server_log.txt 2>&1 &
SERVER_PID=$!

# Wait for servers to initialize
sleep 3

# Check servers are running
pgrep -f "serverM" > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}Servers failed to start!${NC}"
    cat mac_test_results/server_log.txt
    exit 1
fi

echo -e "${GREEN}Servers running.${NC}"

# Auth test
echo -e "\n${YELLOW}Running authentication test...${NC}"
cat > mac_test_results/auth_input.txt << EOF_AUTH
user1
sdvv789
exit
EOF_AUTH

./client < mac_test_results/auth_input.txt > mac_test_results/auth_output.txt 2>&1

grep "granted access" mac_test_results/auth_output.txt > /dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Authentication test passed${NC}"
else
    echo -e "${RED}× Authentication test failed${NC}"
    cat mac_test_results/auth_output.txt
fi

# Quote test
echo -e "\n${YELLOW}Running quote test...${NC}"
cat > mac_test_results/quote_input.txt << EOF_QUOTE
user1
sdvv789
quote
exit
EOF_QUOTE

./client < mac_test_results/quote_input.txt > mac_test_results/quote_output.txt 2>&1

grep "S1" mac_test_results/quote_output.txt > /dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Quote test passed${NC}"
else
    echo -e "${RED}× Quote test failed${NC}"
    cat mac_test_results/quote_output.txt
fi

# Position test
echo -e "\n${YELLOW}Running position test...${NC}"
cat > mac_test_results/position_input.txt << EOF_POSITION
user1
sdvv789
position
exit
EOF_POSITION

./client < mac_test_results/position_input.txt > mac_test_results/position_output.txt 2>&1

# Clean up
echo -e "\n${YELLOW}Cleaning up...${NC}"
kill $SERVER_PID
pkill -f "server[MAPQ]"

echo -e "\n${GREEN}All tests completed. Results in mac_test_results/ directory.${NC}"
echo "To view results: cat mac_test_results/*.txt"
EOF_INNER

chmod +x simplified_test.sh
echo -e "${GREEN}Simplified test script created: simplified_test.sh${NC}"
echo

# Final instructions
echo -e "${YELLOW}${BOLD}===== Mac Test Bundle Ready! =====${NC}${NORMAL}"
echo
echo -e "The following helper scripts have been created:"
echo -e "1. ${BOLD}check_files.sh${NORMAL} - Verifies all required files are present"
echo -e "2. ${BOLD}simplified_test.sh${NORMAL} - Runs basic tests compatible with macOS"
echo
echo -e "${YELLOW}To start the system:${NC}"
echo -e "1. Run ./check_files.sh to verify all files are present"
echo -e "2. Run ./start_servers.sh in one terminal"
echo -e "3. Run ./client in another terminal"
echo
echo -e "${YELLOW}To run tests:${NC}"
echo -e "Run ./simplified_test.sh for basic functionality tests"
echo
echo -e "${GREEN}${BOLD}Enjoy using the Stock Trading System on macOS!${NC}${NORMAL}"
