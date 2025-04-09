#!/bin/bash

# Quick Start Script for EE450 Socket Programming Project
# This script compiles the project, starts all servers, and runs a simple test

echo "====================================="
echo "EE450 Stock Trading System Quick Start"
echo "====================================="

# 1. Compile all components
echo -e "\n[Step 1] Compiling all components..."
make all
if [ $? -ne 0 ]; then
    echo "Compilation failed! Please check the error messages above."
    exit 1
fi
echo "Compilation successful!"

# 2. Start servers
echo -e "\n[Step 2] Starting all servers..."
echo "Killing any existing server processes..."
pkill -f 'server[AMPQ]' 2>/dev/null || true

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

# Verify servers are running
echo "Verifying servers are running..."
SERVER_M_PID=$(pgrep -f "./serverM")
SERVER_A_PID=$(pgrep -f "./serverA")
SERVER_P_PID=$(pgrep -f "./serverP")
SERVER_Q_PID=$(pgrep -f "./serverQ")

if [[ -z "$SERVER_M_PID" || -z "$SERVER_A_PID" || -z "$SERVER_P_PID" || -z "$SERVER_Q_PID" ]]; then
    echo "Error: Some servers failed to start. Please check the log files."
    exit 1
fi

echo "All servers started successfully!"
echo "Server M (Main) running with PID: $SERVER_M_PID"
echo "Server A (Auth) running with PID: $SERVER_A_PID"
echo "Server P (Portfolio) running with PID: $SERVER_P_PID"
echo "Server Q (Quote) running with PID: $SERVER_Q_PID"

# 3. Run a simple test with the client
echo -e "\n[Step 3] Running a simple test with the client..."
echo "Creating test input file..."
cat > test_input.txt << EOF
user1
sdvv789
position
quote
quote S1
exit
EOF

echo "Running client with test input..."
./client < test_input.txt > client_output.txt 2>&1

# 4. Display results
echo -e "\n[Step 4] Test Results:"
echo "===== Client Output ====="
cat client_output.txt
echo "===== End Client Output ====="

echo -e "\nTest complete! You can find more detailed logs in:"
echo "- serverM.log"
echo "- serverA.log"
echo "- serverP.log"
echo "- serverQ.log"
echo "- client_output.txt"

echo -e "\nTo kill all server processes, run: pkill -f 'server[AMPQ]'"
echo "For more tests and functionality verification, please see README.md"