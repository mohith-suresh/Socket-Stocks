#!/bin/bash

# Script to check if all necessary files are present
# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System File Check ====${NC}"

all_files_present=true

# Check source code files
echo -e "${YELLOW}Checking source code files:${NC}"
source_files=("client.cpp" "serverM.cpp" "serverA.cpp" "serverP.cpp" "serverQ.cpp" "Makefile")
for file in "${source_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓ $file${NC}"
    else
        echo -e "${RED}✗ $file${NC}"
        all_files_present=false
    fi
done

# Check data files
echo -e "\n${YELLOW}Checking data files:${NC}"
data_files=("members.txt" "portfolios.txt" "quotes.txt")
for file in "${data_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓ $file${NC}"
    else
        echo -e "${RED}✗ $file${NC}"
        all_files_present=false
    fi
done

# Check script files
echo -e "\n${YELLOW}Checking script files:${NC}"
script_files=("start_servers.sh" "mac_test_bundle.sh" "manual_auth_test.sh" "quick_test.sh")
for file in "${script_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓ $file${NC}"
    else
        echo -e "${RED}✗ $file${NC}"
        all_files_present=false
    fi
done

# Display summary
echo -e "\n${YELLOW}Summary:${NC}"
if [ "$all_files_present" = true ]; then
    echo -e "${GREEN}All necessary files are present.${NC}"
    echo -e "${YELLOW}You can proceed with compilation:${NC}"
    echo -e "  chmod +x mac_test_bundle.sh"
    echo -e "  ./mac_test_bundle.sh"
else
    echo -e "${RED}Some files are missing. Please check the list above.${NC}"
fi