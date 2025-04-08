#!/bin/bash

# This script will test the client against the running servers

echo "Running client tests with username user1, password sdvv789"
echo -e "user1\nsdvv789\nquote\nquote S1\nbuy S1 5\nyes\nsell S1 2\nyes\nposition\nexit\n" | ./client 45000

# Exit with the client's exit code
exit $?