#!/bin/bash

# Kill any existing servers
pkill -f 'server[AMPQ]' 2>/dev/null
sleep 1

# Start all servers
echo "Starting Server M on port 45000..."
./serverM > serverM.log 2>&1 &
sleep 1

echo "Starting Server A..."
./serverA > serverA.log 2>&1 &
sleep 1

echo "Starting Server P..."
./serverP > serverP.log 2>&1 &
sleep 1

echo "Starting Server Q..."
./serverQ > serverQ.log 2>&1 &
sleep 1

# Verify processes are running
echo "Verifying servers are running..."
ps aux | grep 'server[AMPQ]'

echo "All servers are now running!"

# Wait for servers to fully initialize
sleep 2

# Run a comprehensive test with all commands
echo "Running comprehensive tests with username user1, password sdvv789"
echo -e "user1\nsdvv789\nquote\nquote S1\nquote S2\nquote S3\nbuy S1 3\nyes\nsell S2 1\nyes\nposition\nbuy S3 5\nyes\nposition\nexit\n" | ./client 45000

# Kill servers again when done
echo "Cleaning up server processes..."
pkill -f 'server[AMPQ]' 2>/dev/null

# Exit with the client's exit code
exit $?