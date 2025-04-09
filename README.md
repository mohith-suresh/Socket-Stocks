# Stock Trading System

A high-performance stock trading system with advanced distributed networking capabilities, designed for robust and secure real-time trading operations.

## System Architecture

The system consists of four servers:

1. **Main Server (Server M)** - Central hub that connects to clients via TCP and backend servers via UDP
2. **Authentication Server (Server A)** - Handles user authentication 
3. **Portfolio Server (Server P)** - Manages user portfolio data
4. **Quote Server (Server Q)** - Provides stock price quotes

## Port Allocation

- Server M: 45000 (TCP), 44000 (UDP)
- Server A: 41000 (UDP)
- Server P: 42000 (UDP)
- Server Q: 43000 (UDP)

## Key Features

- Multi-server microservice architecture
- Low-level socket programming with strict adherence to Beej's Guide standards
- Secure authentication
- Real-time stock price quotes
- Portfolio management

## Beej's Guide Compliance

This implementation strictly follows Beej's Guide to Network Programming standards:

- Proper socket creation, connection, binding, and error handling
- Correct memory management and resource cleanup
- Signal handling with sigaction
- Robust send/receive wrappers to handle partial sends/receives
- Appropriate socket options (SO_REUSEADDR, timeouts)

## Running the System

### Starting All Servers

```bash
./start_servers.sh
```

This script starts all four servers (M, A, P, Q) and keeps them running.

### Running the Client

```bash
./client
```

### Test Credentials

- Username: `user1`, Password: `sdvv789`
- Username: `user2`, Password: `sdvvzrug`
- Username: `admin`, Password: `vhfuhwsdvv`

## Available Commands

After authentication, the client supports the following commands:

- `position` - View your current portfolio
- `quote` - Get quotes for all stocks
- `quote S1` - Get quote for a specific stock (e.g., S1)
- `buy S1 5` - Buy 5 shares of stock S1
- `sell S1 2` - Sell 2 shares of stock S1
- `exit` - Exit the client

## Testing

Several test scripts are provided:

- `./manual_auth_test.sh` - Interactive test for manual authentication verification
- `./quick_test.sh` - Automated test suite that runs a predefined set of commands
- `./simplified_test.sh` - Basic server startup verification
- `./comprehensive_test.sh` - Complete system test with all functionality

## Implementation Notes

1. The system uses a distributed architecture where each server performs a specialized task
2. Authentication is handled securely with password encryption
3. All network communication follows Beej's Guide standards for reliability and security
4. Error handling is comprehensive throughout the codebase
5. Memory management is carefully implemented to prevent leaks

## Data Files

- `members.txt` - User credentials
- `portfolios.txt` - User stock holdings
- `quotes.txt` - Stock price information