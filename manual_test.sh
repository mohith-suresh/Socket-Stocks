#!/bin/bash

# Manual Test Script for EE450 Socket Programming Project
# This script allows manual testing of specific client operations

echo "====================================="
echo "EE450 Stock Trading System Manual Test"
echo "====================================="

# Check if servers are running
echo -e "\n[Step 1] Checking if servers are running..."
SERVER_M_PID=$(pgrep -f "./serverM")
SERVER_A_PID=$(pgrep -f "./serverA")
SERVER_P_PID=$(pgrep -f "./serverP")
SERVER_Q_PID=$(pgrep -f "./serverQ")

if [[ -z "$SERVER_M_PID" || -z "$SERVER_A_PID" || -z "$SERVER_P_PID" || -z "$SERVER_Q_PID" ]]; then
    echo "Servers not running. Starting them now..."
    
    # Kill any existing servers
    pkill -f 'server[AMPQ]' 2>/dev/null || true
    sleep 1
    
    # Start servers
    echo "Starting Server M (Main)..."
    ./serverM > serverM.log 2>&1 &
    sleep 1
    
    echo "Starting Server A (Authentication)..."
    ./serverA > serverA.log 2>&1 &
    sleep 1
    
    echo "Starting Server P (Portfolio)..."
    ./serverP > serverP.log 2>&1 &
    sleep 1
    
    echo "Starting Server Q (Quote)..."
    ./serverQ > serverQ.log 2>&1 &
    sleep 1
    
    echo "All servers started!"
else
    echo "All servers are already running."
fi

# Display test options
echo -e "\n[Step 2] Choose a test to run:"
echo "1. Authentication Test (with valid credentials)"
echo "2. Authentication Test (with invalid credentials)"
echo "3. Portfolio View Test"
echo "4. Stock Quote Test"
echo "5. Buy Stock Test"
echo "6. Sell Stock Test"
echo "7. Error Handling Test (insufficient shares)"
echo "8. Error Handling Test (invalid stock)"
echo "9. Full Sequence Test"
echo "0. Custom Test (enter your own commands)"

# Get user choice
read -p "Enter your choice (0-9): " choice
echo

case $choice in
    1)
        echo "Running Authentication Test (valid credentials)..."
        cat > test_login.txt << EOF
user1
sdvv789
exit
EOF
        ./client < test_login.txt
        ;;
    2)
        echo "Running Authentication Test (invalid credentials)..."
        cat > test_login.txt << EOF
user1
wrongpassword
EOF
        ./client < test_login.txt
        ;;
    3)
        echo "Running Portfolio View Test..."
        cat > test_login.txt << EOF
user1
sdvv789
position
exit
EOF
        ./client < test_login.txt
        ;;
    4)
        echo "Running Stock Quote Test..."
        cat > test_login.txt << EOF
user1
sdvv789
quote
quote S1
exit
EOF
        ./client < test_login.txt
        ;;
    5)
        echo "Running Buy Stock Test..."
        cat > test_login.txt << EOF
user1
sdvv789
position
buy S1 2
yes
position
exit
EOF
        ./client < test_login.txt
        ;;
    6)
        echo "Running Sell Stock Test..."
        cat > test_login.txt << EOF
user1
sdvv789
position
sell S1 1
yes
position
exit
EOF
        ./client < test_login.txt
        ;;
    7)
        echo "Running Error Handling Test (insufficient shares)..."
        cat > test_login.txt << EOF
user1
sdvv789
position
sell S1 1000
exit
EOF
        ./client < test_login.txt
        ;;
    8)
        echo "Running Error Handling Test (invalid stock)..."
        cat > test_login.txt << EOF
user1
sdvv789
quote NONEXISTENT
exit
EOF
        ./client < test_login.txt
        ;;
    9)
        echo "Running Full Sequence Test..."
        cat > test_login.txt << EOF
user1
sdvv789
position
quote
buy S1 3
yes
position
sell S1 1
yes
position
exit
EOF
        ./client < test_login.txt
        ;;
    0)
        echo "Running Custom Test..."
        echo "Enter your commands below (one per line). Press Ctrl+D when finished."
        echo "First line should be username, second line password, and last line should be 'exit'."
        cat > test_login.txt
        echo "Running client with your custom commands..."
        ./client < test_login.txt
        ;;
    *)
        echo "Invalid choice. Exiting."
        exit 1
        ;;
esac

echo -e "\nTest complete! Check the output above."
echo "To run a different test, execute this script again."
echo "To kill all server processes, run: pkill -f 'server[AMPQ]'"