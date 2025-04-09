# EE450 Socket Programming Project - Stock Trading System

## Project Overview

This system simulates a stock trading platform using socket programming with the following components:
- **Client**: User interface for logging in, viewing stock quotes, buying/selling stocks, and checking portfolio positions
- **Server M (Main)**: Central coordinator that handles client connections and forwards requests to backend servers
- **Server A (Authentication)**: Verifies user credentials against the `members.txt` database
- **Server P (Portfolio)**: Manages user portfolios using the `portfolios.txt` database
- **Server Q (Quote)**: Provides stock quotes using the `quotes.txt` database

All components are implemented following Beej's Guide to Network Programming standards with robust error handling, proper socket management, and clean termination procedures.

## Project Structure

```
.
├── client.cpp         # Client implementation
├── serverM.cpp        # Main server implementation
├── serverA.cpp        # Authentication server implementation
├── serverP.cpp        # Portfolio server implementation
├── serverQ.cpp        # Quote server implementation
├── Makefile           # Compilation instructions
├── members.txt        # User credentials database
├── portfolios.txt     # User portfolio database
├── quotes.txt         # Stock quotes database
├── start_servers.sh   # Script to start all servers
└── README.md          # This file
```

## Compilation

To compile all components, run:

```bash
make all
```

This will compile the client and all server executables.

## Running the System

The system components must be started in a specific order:

### 1. Start All Servers at Once (Recommended)

The easiest way to start all servers is to use the provided script:

```bash
./start_servers.sh
```

This script will:
1. Start Server M (Main Server)
2. Start Server A (Authentication Server)
3. Start Server P (Portfolio Server)
4. Start Server Q (Quote Server)
5. Verify all servers are running

### 2. Start Servers Individually (Alternative)

If you prefer to start each server manually, open separate terminal windows and run:

```bash
# Terminal 1
./serverM

# Terminal 2
./serverA

# Terminal 3
./serverP

# Terminal 4
./serverQ
```

### 3. Start the Client

Once all servers are running, you can start the client in a new terminal:

```bash
./client
```

## Testing All Functionalities

Below are detailed instructions for testing each feature of the system with expected outputs.

### 1. Authentication

**Test Procedure:**
1. Start the servers using `./start_servers.sh`
2. Start the client using `./client`
3. Enter the following credentials:
   - Username: `user1`
   - Password: `sdvv789`

**Expected Output:**
- The client will display: `[Client] You have been granted access.`
- Server M logs will show: `[Server M] Authentication successful for user user1`
- Server A logs will show: `[Server A] Member user1 has been authenticated.`

**Alternative Test:**
- Try incorrect credentials (e.g., username: `user1`, password: `wrong`) to verify authentication fails
- Expected: `[Client] The credentials are incorrect. Please try again.`

### 2. Viewing Portfolio

**Test Procedure:**
1. After successful login, enter the command: `position`

**Expected Output:**
- The client will display the user's portfolio:
```
Your portfolio:
S1: 10 shares, avg price: $105.500000, current price: $100.000000, gain/loss: $-55.000000
S2: 5 shares, avg price: $150.000000, current price: $150.000000, gain/loss: $0.000000
Total unrealized gain/loss: $-55.000000
```
- Server M logs: `[Server M] Received position request from user1`
- Server P logs: `[Server P] Handling portfolio request for user: user1`

### 3. Viewing Stock Quotes

**Test Procedure:**
1. After login, enter: `quote` (to view all stocks)
2. Then enter: `quote S1` (to view a specific stock)

**Expected Output:**
- For `quote` command:
```
Current Stock Prices:
S1: $100.00
S2: $150.00
```

- For `quote S1` command:
```
S1: $100.00
```

- Server M logs: `[Server M] Received quote request for [stock]`
- Server Q logs: `[Server Q] Processing quote request for stock: [stock]`

### 4. Buying Stocks

**Test Procedure:**
1. Enter the command: `buy S1 5` (buy 5 shares of stock S1)
2. When prompted for confirmation, enter: `yes`

**Expected Output:**
- The client will display:
```
You are buying 5 shares of S1 at $100.00 per share for a total of $500.00
Confirm buy? (yes/no): yes
Transaction successful. You now own 15 shares of S1.
```

- Server M logs will show the buy request processing
- Server P logs will show the portfolio update
- Server Q logs will advance the stock price to the next value

### 5. Selling Stocks

**Test Procedure:**
1. Enter the command: `sell S1 3` (sell 3 shares of stock S1)
2. When prompted for confirmation, enter: `yes`

**Expected Output:**
- The client will display:
```
You are selling 3 shares of S1 at $101.00 per share for a total of $303.00
Confirm sell? (yes/no): yes
Transaction successful. You now own 12 shares of S1.
```

- Server logs will show similar processing as with the buy command

### 6. Error Handling

**Test Procedure:**
1. Try to sell more shares than owned: `sell S1 100`
2. Try to query a non-existent stock: `quote NONEXISTENT`

**Expected Output:**
- For excessive sell: `ERROR: Insufficient shares. You only own X shares of S1.`
- For non-existent stock: `ERROR: Stock not found`

### 7. Multiple Users

**Test Procedure:**
1. With servers running, open a second terminal and start another client: `./client`
2. Log in with different credentials:
   - Username: `user2`
   - Password: `12345`
3. Perform operations with this second user

**Expected Output:**
- Both clients should operate independently
- Server M should handle both connections
- Each user should see their own portfolio

### 8. Graceful Termination

**Test Procedure:**
1. Enter the command: `exit` in the client

**Expected Output:**
- The client will display: `[Client] Exiting...`
- The client will terminate
- Server M logs: `[Server M] Client disconnected`

## Socket Programming Features Verification

The implementation includes several socket programming features following Beej's Guide:

### 1. Robust Error Handling

All socket operations include proper error handling with descriptive error messages.

**Verification:**
- Check error messages when servers fail to start (e.g., if a port is already in use)
- Use `kill -9` on a server to see how other components handle the failure

### 2. Signal Handling

Proper signal handling is implemented using `sigaction()` instead of `signal()`.

**Verification:**
- Press Ctrl+C on any server, observe the graceful shutdown messages

### 3. Socket Options

Socket options are configured for better reliability:
- `SO_REUSEADDR` to allow socket reuse
- `SO_RCVTIMEO` for receiving timeouts

**Verification:**
- Restart servers immediately after shutting them down (should work without "Address already in use" errors)

### 4. Robust Send/Receive Functions

Reliable send/receive helper functions handle partial transfers, error conditions, and retries.

**Verification:**
- Observe client behavior when sending large amounts of data
- Test system under network stress

## Troubleshooting

1. **Connection Refused**: Ensure all servers are running in the correct order (M, A, P, Q)
2. **Authentication Failure**: Verify credentials match entries in `members.txt`
3. **Missing Data File**: Ensure `members.txt`, `portfolios.txt`, and `quotes.txt` are in the correct locations
4. **Port Conflicts**: Check if other applications are using the required ports (45000, 44000, 41000, 42000, 43000)

If a server crashes, you'll need to restart it using the commands shown in the "Running the System" section.

## Technical Details

- **Port Allocation**:
  - Server M: 45000 (TCP), 44000 (UDP)
  - Server A: 41000 (UDP)
  - Server P: 42000 (UDP)
  - Server Q: 43000 (UDP)
  - Client: Dynamically assigned (shown during runtime)

- **Communication Protocols**:
  - Client to Server M: TCP
  - Server M to Backend Servers: UDP

- **Password Encryption**:
  - Algorithm: Offset each character/digit by 3 positions
  - Example: `sdvv789` encrypts to `vgyy012`

## Testing Script

For convenience, you can run a predefined test sequence using:

```bash
# Start all servers in the background
./start_servers.sh

# Wait for servers to initialize
sleep 2

# Run client with test sequence (user1/sdvv789, position, quote, exit)
echo -e 'user1\nsdvv789\nposition\nquote\nexit' | ./client
```

This will demonstrate successful authentication, portfolio viewing, and stock quoting features.