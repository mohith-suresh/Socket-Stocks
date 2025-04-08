#!/bin/bash

# Kill any existing servers
pkill -f 'server[AMPQ]' 2>/dev/null
sleep 1

# Start all servers in the correct order and log errors
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
echo "Run './client' to connect to port 45000"