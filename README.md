# Stock Trading System

## Project Overview

This project implements a distributed stock trading system using C++ socket programming. The system consists of a client application and four server components that communicate through TCP and UDP sockets to provide stock trading functionalities.

### System Architecture

- **Client**: Communicates with Server M via TCP connection
- **Server M (Main Server)**: Central hub that connects to clients via TCP and to backend servers via UDP
- **Server A (Authentication Server)**: Handles user authentication requests via UDP
- **Server P (Portfolio Server)**: Manages user portfolio information via UDP
- **Server Q (Quote Server)**: Provides stock quote information via UDP

### Port Allocation Scheme

- **Server M**: TCP port 45000, UDP port 44000
- **Server A**: UDP port 41000
- **Server P**: UDP port 42000
- **Server Q**: UDP port 43000

## Features

- **User Authentication**: Secure login with username/password verification
- **Stock Quotes**: Retrieve pricing information for all stocks or specific stocks
- **Portfolio Management**: View portfolio positions with current market values
- **Trading Operations**: Buy and sell stocks with real-time price quotes
- **Robust Communication**: Reliable message passing with error handling and retry logic

## Technical Implementation

- **Socket Programming**: Uses Beej's Guide to Network Programming best practices
- **Error Handling**: Comprehensive error management with perror() and cleanup routines
- **Signal Handling**: Proper SIGINT handling with sigaction()
- **Message Passing**: Robust send/receive functions with timeout and retry mechanisms
- **Resource Management**: Proper socket cleanup and memory management

## Getting Started

### Compilation

Use the provided Makefile to compile all components:

```bash
make clean all
```

This will compile the client and all server components.

### Starting the Servers

Use the provided startup script to launch all server components:

```bash
chmod +x start_servers.sh
./start_servers.sh
```

### Running the Client

After starting the servers, run the client:

```bash
./client
```

### Test Credentials

- Username: `user1`, Password: `sdvv789`
- Username: `user2`, Password: `sdvvzrug`
- Username: `admin`, Password: `vhfuhwsdvv`

## Available Commands

Once authenticated, the following commands are available:

- `quote`: Show all stock prices
- `quote <stock>`: Show specific stock price
- `buy <stock> <shares>`: Buy shares of a stock
- `sell <stock> <shares>`: Sell shares of a stock
- `position`: View your current portfolio
- `exit`: Logout and exit

## Testing

Several test scripts are provided to verify system functionality:

- `quick_test.sh`: Performs a basic authentication and quote retrieval test
- `auth_only_test.sh`: Tests authentication functionality only
- `quote_test.sh`: Tests stock quote retrieval functionality
- `debug_auth_test.sh`: Detailed authentication test with logging
- `comprehensive_test.sh`: Full system test with all functionalities

## Test Results

See the `test_results` directory for detailed test results and Beej's Guide compliance reports:

- `test_results/test_summary.txt`: Summary of all test results
- `test_results/beejs_compliance_summary.txt`: Detailed report on Beej's Guide compliance

## Implementation Notes

- All network messages are properly null-terminated for reliable transmission
- Robust error handling and retry logic for network operations
- Proper resource cleanup to prevent memory leaks and socket exhaustion
- Signal handling for graceful server shutdown
- Socket options configured for optimal reliability

## Project Files

- `client.cpp`: Client application source
- `serverM.cpp`: Main server implementation
- `serverA.cpp`: Authentication server implementation
- `serverP.cpp`: Portfolio server implementation
- `serverQ.cpp`: Quote server implementation
- `Makefile`: Compilation rules for all components
- `members.txt`: User credentials database
- `portfolios.txt`: User portfolio database
- `quotes.txt`: Stock quote database
- `*.sh`: Various test and utility scripts