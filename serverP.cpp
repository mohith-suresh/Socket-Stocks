/**
 * serverP.cpp - Portfolio Server for Stock Trading Simulation
 * EE450 Socket Programming Project
 * 
 * This server:
 * - Loads user portfolios from portfolios.txt
 * - Manages user stock holdings and transactions
 * - Calculates unrealized gains/losses
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
#define SERVER_P_PORT 42654
#define BUFFER_SIZE 1024
#define PORTFOLIOS_FILE "portfolios.txt"

// Global socket file descriptor for cleanup
int sockfd = -1;

// Data structure for a single stock holding
struct StockHolding {
    std::string stock_name;
    int shares;
    double avg_price;
    
    // Default constructor needed for std::map
    StockHolding() : stock_name(""), shares(0), avg_price(0.0) {}
    
    StockHolding(std::string name, int qty, double price) 
        : stock_name(name), shares(qty), avg_price(price) {}
};

// Portfolio is a map of stock symbols to holdings
typedef std::map<std::string, StockHolding> Portfolio;

// User database maps usernames to portfolios
std::map<std::string, Portfolio> user_portfolios;

// Function prototypes
void sigint_handler(int sig);
void load_portfolios_file();
std::vector<std::string> split_string(const std::string& str, char delimiter);
void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_buy(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_sell(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_check_shares(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_portfolio(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);

// Signal handler for Ctrl+C - Following Beej's Guide Section 9.4 (Signal Handling)
void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    printf("\n[Server P] Caught SIGINT signal, cleaning up and exiting...\n");
    
    if (sockfd != -1) {
        printf("[Server P] Closing socket (fd: %d)...\n", sockfd);
        close(sockfd);
    }
    
    printf("[Server P] Cleanup complete, exiting.\n");
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
        fprintf(stderr, "[Server P] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Server P] Registered signal handler for SIGINT\n");
    
    // Setting up UDP socket with getaddrinfo, socket, and bind
    // Based on Beej's Guide Section 5.3 (Datagram Sockets)
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // Use my IP

    if ((rv = getaddrinfo(NULL, std::to_string(SERVER_P_PORT).c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "[Server P] getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Loop through all the results and bind to the first we can - Following Beej's Guide Section 5.3
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        // Enabling SO_REUSEADDR to allow address reuse
        // Based on Beej's Guide Section 7.1 (setsockopt())
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
        fprintf(stderr, "[Server P] Failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    
    printf("[Server P] Socket options set successfully\n");
    
    // Load portfolios
    load_portfolios_file();
    
    printf("[Server P] Booting up using UDP on port %d\n", SERVER_P_PORT);
    
    // Main server loop - Following Beej's Guide Section 5.3 (Datagram Sockets)
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int numbytes;

    while (1) {
        addr_len = sizeof their_addr;
        // Receiving messages via UDP
        // Based on Beej's Guide Section 5.8 (recvfrom — DGRAM-style)
        if ((numbytes = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            continue;
        }

        buffer[numbytes] = '\0';
        printf("[Server P] Received message: %s\n", buffer);
        
        process_message(buffer, (struct sockaddr_in*)&their_addr, addr_len);
    }
    
    return 0;
}

void load_portfolios_file() {
    std::ifstream file(PORTFOLIOS_FILE);
    
    if (!file.is_open()) {
        printf("[Server P] Error: Could not open portfolios file: %s\n", PORTFOLIOS_FILE);
        exit(1);
    }
    
    std::string line;
    std::string current_user;
    int user_count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        std::vector<std::string> parts = split_string(line, ' ');
        
        if (parts.size() == 1) {
            // This is a username line
            current_user = parts[0];
            user_portfolios[current_user] = Portfolio();
            user_count++;
        } 
        else if (parts.size() == 3 && !current_user.empty()) {
            // This is a stock holding line
            std::string stock_name = parts[0];
            int shares = std::stoi(parts[1]);
            double avg_price = std::stod(parts[2]);
            
            user_portfolios[current_user][stock_name] = StockHolding(stock_name, shares, avg_price);
        }
    }
    
    file.close();
    printf("[Server P] Loaded %d user portfolios from %s\n", user_count, PORTFOLIOS_FILE);
}

void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string msg(message);
    std::vector<std::string> parts = split_string(msg, ' ');
    
    if (parts.size() < 1) {
        printf("[Server P] Error: Received empty message\n");
        return;
    }
    
    printf("[Server P] Processing message type: %s with %zu parts\n", parts[0].c_str(), parts.size());
    
    if (parts[0] == "BUY" && parts.size() == 5) {
        handle_buy(parts, client_addr, client_len);
    } 
    else if (parts[0] == "SELL" && parts.size() == 5) {
        handle_sell(parts, client_addr, client_len);
    } 
    else if (parts[0] == "CHECK" && parts.size() == 4) {
        handle_check_shares(parts, client_addr, client_len);
    } 
    else if (parts[0] == "PORTFOLIO" && parts.size() == 2) {
        printf("[Server P] Handling portfolio request for user: %s\n", parts[1].c_str());
        handle_portfolio(parts, client_addr, client_len);
    }
    else {
        printf("[Server P] Error: Unrecognized message format: '%s'\n", msg.c_str());
    }
}

void handle_buy(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    if (parts.size() != 5) {
        const char* error = "ERROR: Invalid BUY format";
        sendto(sockfd, error, strlen(error), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    std::string username = parts[1];
    std::string stock_name = parts[2];
    int num_shares = std::stoi(parts[3]);
    double price = std::stod(parts[4]);
    
    printf("[Server P] Processing buy request: %s buying %d shares of %s at $%.2f\n", 
           username.c_str(), num_shares, stock_name.c_str(), price);
    
    // Create user portfolio if it doesn't exist
    if (user_portfolios.find(username) == user_portfolios.end()) {
        user_portfolios[username] = Portfolio();
    }
    
    Portfolio& portfolio = user_portfolios[username];
    
    // Check if user already has this stock
    if (portfolio.find(stock_name) == portfolio.end()) {
        // New stock for this user
        portfolio[stock_name] = StockHolding(stock_name, num_shares, price);
    } else {
        // Update existing holding
        StockHolding& holding = portfolio[stock_name];
        
        // Calculate new average price
        double old_value = holding.shares * holding.avg_price;
        double new_value = num_shares * price;
        int total_shares = holding.shares + num_shares;
        
        holding.avg_price = (old_value + new_value) / total_shares;
        holding.shares = total_shares;
    }
    
    // Prepare response with the required format
    std::string response = "BUY_SUCCESS " + username + " " + stock_name + " " + 
                          std::to_string(num_shares) + " " + std::to_string(price);
    
    // Sending messages via UDP - Based on Beej's Guide Section 5.8 (sendto — DGRAM-style)
    if (sendto(sockfd, response.c_str(), response.length(), 0,
             (struct sockaddr *)client_addr, client_len) == -1) {
        perror("sendto");
    }
}

void handle_sell(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string username = parts[1];
    std::string stock_name = parts[2];
    int num_shares = std::stoi(parts[3]);
    double price = std::stod(parts[4]);
    
    printf("[Server P] Processing sell request: %s selling %d shares of %s at $%.2f\n", 
           username.c_str(), num_shares, stock_name.c_str(), price);
    
    // Check if user exists
    if (user_portfolios.find(username) == user_portfolios.end()) {
        const char* response = "ERROR: User portfolio not found";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    Portfolio& portfolio = user_portfolios[username];
    
    // Check if user has the stock
    if (portfolio.find(stock_name) == portfolio.end() || 
        portfolio[stock_name].shares < num_shares) {
        const char* response = "ERROR: Insufficient shares";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    // Update holding
    StockHolding& holding = portfolio[stock_name];
    holding.shares -= num_shares;
    
    // Calculate profit/loss from this transaction
    double profit = num_shares * (price - holding.avg_price);
    
    // Prepare response
    std::string response = "SELL_CONFIRMED: " + std::to_string(num_shares) + 
                          " shares of " + stock_name + " at $" + std::to_string(price) + 
                          ", profit/loss: $" + std::to_string(profit);
    
    // Sending messages via UDP
    // Based on Beej's Guide Section 5.8 (sendto — DGRAM-style)
    if (sendto(sockfd, response.c_str(), response.length(), 0,
             (struct sockaddr *)client_addr, client_len) == -1) {
        perror("sendto");
    }
}

void handle_check_shares(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string username = parts[1];
    std::string stock_name = parts[2];
    int num_shares = std::stoi(parts[3]);
    
    printf("[Server P] Checking if %s has %d shares of %s\n", 
           username.c_str(), num_shares, stock_name.c_str());
    
    // Check if user exists
    if (user_portfolios.find(username) == user_portfolios.end()) {
        const char* response = "INSUFFICIENT_SHARES";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    Portfolio& portfolio = user_portfolios[username];
    
    // Check if user has enough shares
    if (portfolio.find(stock_name) == portfolio.end() || 
        portfolio[stock_name].shares < num_shares) {
        const char* response = "INSUFFICIENT_SHARES";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    // User has enough shares
    const char* response = "SUFFICIENT_SHARES";
    sendto(sockfd, response, strlen(response), 0,
         (struct sockaddr *)client_addr, client_len);
}

void handle_portfolio(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string username = parts[1];
    
    printf("[Server P] Retrieving portfolio for %s\n", username.c_str());
    
    // Check if user exists
    if (user_portfolios.find(username) == user_portfolios.end()) {
        printf("[Server P] User %s not found in portfolios\n", username.c_str());
        std::string response = "PORTFOLIO\n";
        printf("[Server P] Sending empty portfolio response (user not found): \n%s\n", response.c_str());
        printf("[Server P] Response length: %zu bytes\n", response.length());
        sendto(sockfd, response.c_str(), response.length(), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    Portfolio& portfolio = user_portfolios[username];
    printf("[Server P] Found portfolio with %zu stocks for user %s\n", portfolio.size(), username.c_str());
    
    // Prepare portfolio response
    std::string response = "PORTFOLIO\n";
    int stocks_added = 0;
    
    for (const auto& holding : portfolio) {
        const StockHolding& stock = holding.second;
        
        // Only include stocks with shares > 0
        if (stock.shares > 0) {
            printf("[Server P] Adding stock %s with %d shares at avg price %.2f\n", 
                  stock.stock_name.c_str(), stock.shares, stock.avg_price);
            response += stock.stock_name + " " + 
                      std::to_string(stock.shares) + " " + 
                      std::to_string(stock.avg_price) + "\n";
            stocks_added++;
        } else {
            printf("[Server P] Skipping stock %s (0 shares)\n", stock.stock_name.c_str());
        }
    }
    
    printf("[Server P] Added %d stocks to the response\n", stocks_added);
    printf("[Server P] Final portfolio response (%zu bytes): \n%s\n", response.length(), response.c_str());
    
    // Sending messages via UDP
    // Based on Beej's Guide Section 5.8 (sendto — DGRAM-style)
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
