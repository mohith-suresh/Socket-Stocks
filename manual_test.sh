#!/bin/bash

echo "======================="
echo "Stock Trading Test Tool"
echo "======================="
echo "Starting all server components..."

# Kill any existing servers
pkill -9 -f "server[MAPQ]" 2>/dev/null || true
sleep 1

# Start all servers
./serverM > /dev/null 2>&1 &
./serverA > /dev/null 2>&1 &
./serverP > /dev/null 2>&1 &
./serverQ > /dev/null 2>&1 &
sleep 1

echo ""
echo "All servers started! Now you can manually test the client."
echo ""
echo "Test credentials:"
echo " - Username: user1  Password: sdvv789"
echo " - Username: user2  Password: sdvvzrug"
echo " - Username: admin  Password: vhfuhwsdvv"
echo ""
echo "Test commands:"
echo " - quote (shows all stocks)"
echo " - quote S1 (shows specific stock)"
echo " - buy S1 5 (buy 5 shares of S1)"
echo " - sell S1 2 (sell 2 shares of S1)"
echo " - position (view portfolio)"
echo " - exit (logout)"
echo ""
echo "Starting client - interact with it below:"
echo "======================="

# Run client for interactive testing
./client

# Clean up servers when done
echo ""
echo "Client session ended, cleaning up servers..."
pkill -9 -f "server[MAPQ]" 2>/dev/null
echo "All servers stopped."