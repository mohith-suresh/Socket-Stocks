# EE450 Socket‑Programming Project Ubuntu 22.04 ARM64 for M1/M2 Mac

**Student:** Mohith S  
**USC ID:** 1486084654

---

## Program Overview
This project implements the stock‑trading system described in the Spring 2025 Rev 02 spec:

* Five independent processes (`serverM`, `serverA`, `serverP`, `serverQ`, `client`).
* Handles `quote`, `buy`, `sell`, `position`, and `exit` requests correctly.
* Encrypts passwords on login using a +3 cyclic shift.
* All displayed messages exactly match the spec tables word-for-word.
* Discovers dynamic ports using `getsockname()`; static ports follow the *last three digits* rule.

---

### How messages are sent
* `AUTH <username> <password>` – plain password from client to Server M  
* `AUTH <username> <encrypted_pw>` – same, but the password is the +3‐shift cipher when Server M talks to Server A  
* Trading commands (`quote`, `buy <stock> <shares>`, `sell <stock> <shares>`, `position`) are plain space‑separated strings.  
  No commas or binary fields are used.  Every UDP/TCP payload is a null‑terminated ASCII line.

## Source files

```
client.cpp: Interactive terminal interface for members (login, trading commands). Uses TCP to `serverM`. 
serverM.cpp: Main coordinator: handles client TCP connections, communicates with backend servers over UDP, and enforces trading logic.
serverA.cpp: Authentication server – stores `members.txt`, validates encrypted credentials.
serverP.cpp: Portfolio server – maintains holdings, average buy prices, profit/loss; executes BUY/SELL updates.
serverQ.cpp: Quote server – manages rolling price list (`quotes.txt`), returns current quotes, and handles time‑shift requests.
Makefile: Builds all five executables (`make all`) or cleans them (`make clean`).
```

All functionalities and output messages are happening as per the specifications.

---

## Building & running

```bash
make clean
make all   # Compile all source files

# In five separate terminals
./serverM
./serverA
./serverP
./serverQ
./client
```


---

### Re‑used code
* Basic socket boiler‑plate for example setup, getaddrinfo, loops, etc, inspired from Beej’s Guide to Network Programming.

## Notes for the graders
* Developed and tested on the course's **Ubuntu 22.04 ARM64 for M1/M2 Mac** using `g++ -std=c++11`.
* Programs keep running until you press **Ctrl‑C** (servers) or type **exit** in the client.
