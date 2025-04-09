#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${YELLOW}==== Stock Trading System: Running All Tests ====${NC}"

# Function to run a test and track its result
run_test() {
    local test_name=$1
    local script_path=$2
    
    echo -e "\n${MAGENTA}=== Running Test: ${test_name} ===${NC}"
    chmod +x ${script_path}
    timeout 20 ${script_path}
    local result=$?
    
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}✅ ${test_name} - PASSED${NC}"
        return 0
    elif [ $result -eq 124 ]; then
        echo -e "${RED}❌ ${test_name} - TIMEOUT${NC}"
        return 1
    else
        echo -e "${RED}❌ ${test_name} - FAILED (code: ${result})${NC}"
        return 1
    fi
}

# Record our test results
TOTAL_TESTS=0
PASSED_TESTS=0

# Run each test script
TESTS=(
    "Quick Test:./quick_test.sh"
    "Authentication Test:./auth_only_test.sh"
    "Quote Test:./quote_test.sh"
    "Authentication Debug Test:./debug_auth_test.sh"
    "Client Direct Test:./client < auth_only.txt"
)

for test_info in "${TESTS[@]}"; do
    TEST_NAME="${test_info%%:*}"
    TEST_SCRIPT="${test_info#*:}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    run_test "$TEST_NAME" "$TEST_SCRIPT"
    if [ $? -eq 0 ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
    
    # Brief pause between tests
    sleep 2
done

# Print summary
echo -e "\n${BLUE}====== TEST SUMMARY ======${NC}"
echo -e "Total tests run: ${TOTAL_TESTS}"
echo -e "Tests passed: ${PASSED_TESTS}"
echo -e "Tests failed: $((TOTAL_TESTS - PASSED_TESTS))"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "\n${GREEN}🎉 ALL TESTS PASSED! The system is working correctly.${NC}"
    exit 0
else
    echo -e "\n${RED}⚠️ SOME TESTS FAILED. Please check individual test outputs.${NC}"
    exit 1
fi