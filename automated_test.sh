#!/bin/bash

# Create a named pipe
PIPE_NAME="/tmp/client_pipe"
rm -f $PIPE_NAME
mkfifo $PIPE_NAME

# Run client with input from named pipe as a background process
cat $PIPE_NAME | ./client > client_output.txt 2>&1 &
CLIENT_PID=$!

# Wait for client to start
sleep 1

# Write commands to pipe
echo "user1" > $PIPE_NAME
echo "sdvv789" > $PIPE_NAME
echo "position" > $PIPE_NAME
echo "quote S1" > $PIPE_NAME
echo "buy S1 2" > $PIPE_NAME
echo "yes" > $PIPE_NAME
echo "position" > $PIPE_NAME
echo "exit" > $PIPE_NAME

# Wait for client to finish
wait $CLIENT_PID

# Clean up
rm -f $PIPE_NAME

# Display output
echo "=== TEST COMPLETE. CLIENT OUTPUT: ==="
cat client_output.txt