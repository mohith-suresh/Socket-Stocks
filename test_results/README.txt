EE450 Socket Programming Project - Test Results Documentation

This file contains example outputs from various test cases to demonstrate the correct functioning of the stock trading system. Compare your outputs with these examples to verify system functionality.

===========================
TEST CASE 1: AUTHENTICATION
===========================

Command: Start all servers and client; login with username "user1", password "sdvv789"

Expected Client Output:
-----------------------
[Client] Registered signal handler for SIGINT
[Client] Socket created successfully (fd: 3)
[Client] Attempting to connect to 127.0.0.1:45000
[Client] Connected to Main Server using TCP on port 41818
[Client] Enter username: user1
[Client] Enter password: sdvv789
[Client] Authentication response: AUTH_SUCCESS
[Client] Authentication successful!

Expected Server M Output:
------------------------
[Server M] Accepted new client connection from 127.0.0.1:41818
[Server M] Handling client directly (no fork)
[Server M] Received authentication request for user user1
[Server M] Encrypted password for authentication
[Server M] Authentication successful for user user1

Expected Server A Output:
-------------------------
[Server A] Received message: AUTH user1 vgyy012
[Server A] Received username user1 and password ******.
[Server A] Member user1 has been authenticated.

=========================
TEST CASE 2: VIEW PORTFOLIO
=========================

Command: After login, enter "position"

Expected Client Output:
-----------------------
[Client] Executing command: position
Your portfolio:
S1: 10 shares, avg price: $105.500000, current price: $100.000000, gain/loss: $-55.000000
S2: 5 shares, avg price: $150.000000, current price: $150.000000, gain/loss: $0.000000
Total unrealized gain/loss: $-55.000000

Expected Server M Output:
------------------------
[Server M] Received position request from user1
[Server M] Sending portfolio request to Server P: 'PORTFOLIO user1'
[Server M] Waiting for portfolio response from Server P...
[Server M] Received portfolio response from Server P: 
PORTFOLIO
S1 10 105.500000
S2 5 150.000000
[Server M] Split portfolio response into 3 lines

Expected Server P Output:
------------------------
[Server P] Received message: PORTFOLIO user1
[Server P] Processing message type: PORTFOLIO with 2 parts
[Server P] Handling portfolio request for user: user1
[Server P] Retrieving portfolio for user1
[Server P] Found portfolio with 2 stocks for user user1
[Server P] Adding stock S1 with 10 shares at avg price 105.50
[Server P] Adding stock S2 with 5 shares at avg price 150.00
[Server P] Added 2 stocks to the response

============================
TEST CASE 3: VIEW STOCK QUOTES
============================

Command: After login, enter "quote" (all stocks) or "quote S1" (specific stock)

Expected Client Output for "quote S1":
--------------------------------------
[Client] Executing command: quote S1
S1: $100.00

Expected Server M Output:
------------------------
[Server M] Received quote request for S1

Expected Server Q Output:
------------------------
[Server Q] Received message: QUOTE S1
[Server Q] Processing quote request for stock: S1

====================
TEST CASE 4: BUY STOCK
====================

Command: Enter "buy S1 5"

Expected Client Output:
-----------------------
[Client] Executing command: buy S1 5
You are buying 5 shares of S1 at $100.00 per share for a total of $500.00
Confirm buy? (yes/no): yes
Transaction successful. You now own 15 shares of S1.

Expected Server M Output:
------------------------
[Server M] Processing buy request for stock S1, 5 shares
[Server M] Getting current price for S1
[Server M] Received price $100.00 for S1
[Server M] Confirming buy: 5 shares of S1 at $100.00
[Server M] Processing portfolio update for user1

Expected Server P Output:
------------------------
[Server P] Updating portfolio for user user1
[Server P] Adding 5 shares of S1 at $100.00

Expected Server Q Output:
------------------------
[Server Q] Processing quote request for stock: S1
[Server Q] Advancing price index for S1

=====================
TEST CASE 5: SELL STOCK
=====================

Command: Enter "sell S1 3"

Expected Client Output:
-----------------------
[Client] Executing command: sell S1 3
You are selling 3 shares of S1 at $101.00 per share for a total of $303.00
Confirm sell? (yes/no): yes
Transaction successful. You now own 12 shares of S1.

Expected Server M Output:
------------------------
[Server M] Processing sell request for stock S1, 3 shares
[Server M] Getting current price for S1
[Server M] Received price $101.00 for S1
[Server M] Confirming sell: 3 shares of S1 at $101.00
[Server M] Processing portfolio update for user1

Expected Server P Output:
------------------------
[Server P] Updating portfolio for user user1
[Server P] Removing 3 shares of S1

Expected Server Q Output:
------------------------
[Server Q] Processing quote request for stock: S1
[Server Q] Advancing price index for S1

======================
TEST CASE 6: ERROR CASES
======================

Command: Try to buy/sell non-existent stock "quote GOOG"

Expected Client Output:
-----------------------
[Client] Executing command: quote GOOG
ERROR: Stock not found

Expected Server Q Output:
------------------------
[Server Q] Received message: QUOTE GOOG
[Server Q] Processing quote request for stock: GOOG
[Server Q] Stock not found

Command: Try to sell more shares than owned "sell S1 100"

Expected Client Output:
-----------------------
[Client] Executing command: sell S1 100
ERROR: Insufficient shares. You only own 12 shares of S1.

Expected Server M Output:
------------------------
[Server M] Processing sell request for stock S1, 100 shares
[Server M] User user1 has insufficient shares (has 12, attempting to sell 100)

==========================
TEST CASE 7: GRACEFUL EXIT
==========================

Command: Enter "exit"

Expected Client Output:
-----------------------
[Client] Executing command: exit
[Client] Exiting...

Expected Server M Output:
------------------------
[Server M] Client disconnected
[Server M] Client connection closed, waiting for new connections

===============================
TEST CASE 8: MULTIPLE CLIENTS
===============================

Command: Run two clients simultaneously with different users

Expected behavior:
- Both clients can authenticate and perform operations independently
- Each client's operations don't affect the other's session
- Server M handles multiple client connections correctly
- Backend servers process requests from Server M regardless of which client initiated them