#!/usr/bin/env bash
#
# 1) rebuild everything
make clean
make all || { echo "build failed"; exit 1; }

# 2) launch each server in the background, redirecting logs
./serverM                > serverM.log 2>&1 &
./serverA                > serverA.log 2>&1 &
./serverP                > serverP.log 2>&1 &
./serverQ                > serverQ.log 2>&1 &

# 3) give them a moment to bind their ports
sleep 1

# 4) finally start the client in the foreground
./client