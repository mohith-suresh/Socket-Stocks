#!/bin/bash

# Stop any running servers
pkill -f "./serverM" 2>/dev/null || true
pkill -f "./serverA" 2>/dev/null || true
pkill -f "./serverP" 2>/dev/null || true
pkill -f "./serverQ" 2>/dev/null || true
sleep 1

# Start only the main server with verbose output
echo "Starting Server M in verbose mode..."
./serverM > serverM.log 2>&1 &
MAIN_PID=$!
sleep 2

# Check if it's running
if ps -p $MAIN_PID > /dev/null; then
    echo "Server M is running with PID $MAIN_PID"
else
    echo "Server M failed to start!"
    cat serverM.log
    exit 1
fi

# Let it run for a few seconds
echo "Server M is running... waiting 5 seconds"
sleep 5

# Check log
echo "Server M log output:"
cat serverM.log

# Kill server
kill $MAIN_PID
echo "Test completed, server terminated."