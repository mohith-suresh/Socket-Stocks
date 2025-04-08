#!/bin/bash

# Start servers if needed
if ! pgrep -f "serverM" > /dev/null; then
    echo "Starting servers..."
    ./start_servers.sh
    sleep 2
fi

# Run client with input script
echo -e "user1\nsdvv789\nquote\nquote S1\nbuy S1 10\nyes\nposition\nsell S1 5\nyes\nposition\nexit\n" | ./client