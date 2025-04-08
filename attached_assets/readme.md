# EE450 Socket Programming Project - Stock Trading Simulation

## Recent Updates (March 28, 2025)
- Fixed bug in buy command that was causing "ERROR: Invalid portfolio response"
- Fixed sell command price display issue (now shows correct price)
- Improved handling of responses in ServerM for more reliable operation
- Added detailed debug logging throughout the system
- Created new comprehensive test script

## Overview
This project implements a simplified stock trading simulator with multiple server components and a client interface. The system allows users to authenticate, check stock quotes, buy/sell stocks, and view their portfolio with unrealized gains/losses.

## Components

### Client
- Provides a terminal-based interface for users to interact with the trading system
- Connects to Server M via TCP
- Supports commands: quote, buy, sell, position, exit

### Server M (Main Server)
- Central coordinator that routes client requests to appropriate backend servers
- Communicates with clients via TCP and with backend servers via UDP
- Manages the flow of data between clients and backend services

### Server A (Authentication)
- Verifies user credentials using the members.txt file
- Handles username/password authentication requests
- Implements the required password encryption scheme

### Server P (Portfolio)
- Manages user stock holdings using the portfolios.txt file
- Processes buy and sell transactions
- Tracks user positions and calculates gains/losses

### Server Q (Quote)
- Provides current stock prices from quotes.txt
- Supports both individual and all-stock quote requests
- Implements cyclic advancement of stock prices after transactions

## Message Format

### Authentication
- Client to Server M: `AUTH <username> <password>`
- Server M to Server A: `AUTH <username> <encrypted_password>`
- Response: `AUTH_SUCCESS` or `AUTH_FAILED`

### Quote
- Client to Server M: `quote [stock_name]`
- Server M to Server Q: `QUOTE [stock_name]`
- Response: `<stock_name> <price>` or list of stocks with prices

### Buy/Sell
- Client to Server M: `buy <stock_name> <num_shares>` or `sell <stock_name> <num_shares>`
- Server M gets price from Server Q: `QUOTE <stock_name>`
- For sell, Server M checks with Server P: `CHECK <username> <stock_name> <num_shares>`
- User confirmation required
- Server M to Server P: `BUY/SELL <username> <stock_name> <num_shares> <price>`
- Server M to Server Q: `ADVANCE <stock_name>`

### Portfolio
- Client to Server M: `position`
- Server M to Server P: `PORTFOLIO <username>`
- Server M gets current prices from Server Q for each stock
- Response: Formatted portfolio with current gains/losses

## Technical Implementation

### Port Allocation
- Server A: UDP 41XXX
- Server P: UDP 42XXX
- Server Q: UDP 43XXX
- Server M (UDP): 44XXX
- Server M (TCP): 45XXX
- Clients: Dynamic ports

### Data Files
- members.txt: User authentication credentials
- portfolios.txt: User stock holdings
- quotes.txt: Stock price sequences

### Known Limitations
- No persistent storage - portfolios are held in memory and lost on server restart
- Limited error handling for concurrent requests
- No transaction history tracking

## Usage Instructions
1. Compile all components:
   ```
   make all
   ```

2. Use the interactive testing menu (recommended):
   ```
   make menu
   ```
   This provides a menu-driven interface for testing the system.

3. Or manually start the servers in the following order:
   - `./serverM`
   - `./serverA`
   - `./serverP`
   - `./serverQ`

4. Start the client with `./client`

5. Log in with valid credentials (e.g., username: `user1`, password: `sdvv789`)

6. Use the available commands to interact with the system:
   - `quote` - View all stock prices
   - `quote <stock>` - View specific stock price
   - `buy <stock> <shares>` - Buy shares of a stock
   - `sell <stock> <shares>` - Sell shares of a stock
   - `position` - View your portfolio with gains/losses
   - `exit` - Logout

## Testing
The project includes several testing scripts:

1. Comprehensive Testing (Recommended):
   ```
   ./comprehensive_test.sh
   ```
   - Starts all servers
   - Runs a sequence of commands covering all functionality
   - Provides detailed test results with pass/fail status
   - Use `./comprehensive_test.sh --verbose` for complete output

2. Quick Test:
   ```
   ./auto_test.sh
   ```
   - Basic automated testing of core functionality
   - Shows client output and error logs
   - Less detailed than comprehensive test

3. Manual Client Testing:
   ```
   ./manual_client_test.sh
   ```
   - Starts servers and runs a predefined client scenario
   - Useful for observing behavior step by step

4. Simplified Test (For Specific Commands):
   ```
   ./simplified_test.sh
   ```
   - Focused testing of specific commands
   - Modify the script to test different scenarios

## Code Structure
- Each server is implemented as a separate executable
- Common utilities like string splitting are repeated in each file for modularity
- The code follows a request-response pattern for all operations
- Testing scripts help verify system functionality
