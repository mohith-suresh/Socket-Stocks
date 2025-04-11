/**
 * serverQ.cpp - Quote Server for Stock Trading Simulation
 * EE450 Socket Programming Project
 * 
 * This server:
 * - Loads stock quotes from quotes.txt
 * - Provides current stock prices in response to quote requests
 * - Advances stock price index after buy/sell transactions
 * - Communicates with Server M via UDP
 */

// Portions of this code are based on Beej's Guide to Network Programming (v3.2.1)
// https://beej.us/guide/bgnet/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>

// Default values - replace XXX with your USC ID last 3 digits
#define SERVER_Q_PORT 43654
#define BUFFER_SIZE 1024
#define QUOTES_FILE "quotes.txt"
#define MAX_PRICES 10 // Each stock has 10 prices that cycle

// Global socket file descriptor for cleanup
int sockfd = -1;

// Stock data structures
struct StockQuote {
    std::string name;
    double prices[MAX_PRICES];
    int current_idx;
    
    StockQuote() : current_idx(0) {
        for (int i = 0; i < MAX_PRICES; i++) {
            prices[i] = 0.0;
        }
    }
};

std::map<std::string, StockQuote> stock_quotes;

// Function prototypes
void sigint_handler(int sig);
void load_quotes_file();
std::vector<std::string> split_string(const std::string& str, char delimiter);
void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_quote(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_advance(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);

// Signal handler for Ctrl+C - Following Beej's Guide Section 9.4 (Signal Handling)
void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    printf("\n[Server Q] Caught SIGINT signal, cleaning up and exiting...\n");
    
    if (sockfd != -1) {
        printf("[Server Q] Closing socket (fd: %d)...\n", sockfd);
        close(sockfd);
    }
    
    printf("[Server Q] Cleanup complete, exiting.\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    // Graceful shutdown with SIGINT - Based on Beej's Guide Section 9.4 (Signal Handling)
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    // Not using SA_RESTART to ensure interrupted system calls fail with EINTR
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        fprintf(stderr, "[Server Q] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Server Q] Registered signal handler for SIGINT\n");
    
    // Setting up UDP socket with getaddrinfo, socket, and bind
    // Based on Beej's Guide Section 5.3 (Datagram Sockets)
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // Use my IP

    if ((rv = getaddrinfo(NULL, std::to_string(SERVER_Q_PORT).c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "[Server Q] getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Loop through all the results and bind to the first we can - Following Beej's Guide Section 5.3
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        // Enabling SO_REUSEADDR - Based on Beej's Guide Section 7.1 (setsockopt())
        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt SO_REUSEADDR");
            close(sockfd);
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "[Server Q] Failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    
    printf("[Server Q] Socket options set successfully\n");
    
    // Load quotes
    load_quotes_file();
    
    printf("[Server Q] Booting up using UDP on port %d\n", SERVER_Q_PORT);
    
    // Main server loop - Following Beej's Guide Section 5.3 (Datagram Sockets)
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int numbytes;

    while (1) {
        addr_len = sizeof their_addr;
        // Receiving messages via UDP - Based on Beej's Guide Section 5.8 (recvfrom — DGRAM-style)
        if ((numbytes = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            continue;
        }

        buffer[numbytes] = '\0';
        printf("[Server Q] Received message: %s\n", buffer);
        
        process_message(buffer, (struct sockaddr_in*)&their_addr, addr_len);
    }
    
    return 0;
}

void load_quotes_file() {
    std::ifstream file(QUOTES_FILE);
    
    if (!file.is_open()) {
        printf("[Server Q] Error: Could not open quotes file: %s\n", QUOTES_FILE);
        exit(1);
    }
    
    std::string line;
    int stock_count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        std::vector<std::string> parts = split_string(line, ' ');
        
        if (parts.size() >= MAX_PRICES + 1) {
            std::string stock_name = parts[0];
            StockQuote quote;
            quote.name = stock_name;
            
            for (int i = 0; i < MAX_PRICES; i++) {
                quote.prices[i] = std::stod(parts[i + 1]);
            }
            
            stock_quotes[stock_name] = quote;
            stock_count++;
        }
    }
    
    file.close();
    printf("[Server Q] Loaded %d stock quotes from %s\n", stock_count, QUOTES_FILE);
}

void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string msg(message);
    std::vector<std::string> parts = split_string(msg, ' ');
    
    if (parts.size() < 1) {
        return;
    }
    
    if (parts[0] == "QUOTE") {
        handle_quote(parts, client_addr, client_len);
    } 
    else if (parts[0] == "ADVANCE" && parts.size() == 2) {
        handle_advance(parts, client_addr, client_len);
    }
}

void handle_quote(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    if (parts.size() == 1) {
        // Request for all stock quotes
        printf("[Server Q] Processing request for all stock quotes\n");
        
        std::string response;
        
        for (const auto& stock_pair : stock_quotes) {
            const StockQuote& quote = stock_pair.second;
            double current_price = quote.prices[quote.current_idx];
            
            response += quote.name + " " + std::to_string(current_price) + "\n";
        }
        
        // Sending messages via UDP - Based on Beej's Guide Section 5.8 (sendto — DGRAM-style)
        if (sendto(sockfd, response.c_str(), response.length(), 0,
                 (struct sockaddr *)client_addr, client_len) == -1) {
            perror("sendto");
        }
    } 
    else if (parts.size() == 2) {
        // Request for specific stock quote
        std::string stock_name = parts[1];
        
        printf("[Server Q] Processing quote request for stock: %s\n", stock_name.c_str());
        
        // Check if stock exists
        if (stock_quotes.find(stock_name) == stock_quotes.end()) {
            const char* error = "ERROR: Stock not found";
            sendto(sockfd, error, strlen(error), 0,
                 (struct sockaddr *)client_addr, client_len);
            return;
        }
        
        // Get current price
        const StockQuote& quote = stock_quotes[stock_name];
        double current_price = quote.prices[quote.current_idx];
        
        // Prepare response
        std::string response = stock_name + " " + std::to_string(current_price);
        
        // Sending messages via UDP - Based on Beej's Guide Section 5.8 (sendto — DGRAM-style)
        if (sendto(sockfd, response.c_str(), response.length(), 0,
                 (struct sockaddr *)client_addr, client_len) == -1) {
            perror("sendto");
        }
    }
}

void handle_advance(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string stock_name = parts[1];
    
    // Check if stock exists
    if (stock_quotes.find(stock_name) == stock_quotes.end()) {
        const char* error = "ERROR: Stock not found";
        sendto(sockfd, error, strlen(error), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    // Advance stock price index
    StockQuote& quote = stock_quotes[stock_name];
    int old_idx = quote.current_idx;
    quote.current_idx = (quote.current_idx + 1) % MAX_PRICES;
    
    double price = quote.prices[old_idx]; // Price before advancing
    
    printf("[Server Q] Received a time forward request for %s, the current price of that stock is %.2f at time %d.\n",
           stock_name.c_str(), price, old_idx);
    
    // Prepare response
    std::string response = "ADVANCED " + stock_name + " to index " + 
                          std::to_string(quote.current_idx) + 
                          ", new price: " + std::to_string(quote.prices[quote.current_idx]);
    
    // Sending messages via UDP - Based on Beej's Guide Section 5.8 (sendto — DGRAM-style)
    if (sendto(sockfd, response.c_str(), response.length(), 0,
             (struct sockaddr *)client_addr, client_len) == -1) {
        perror("sendto");
    }
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}
