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
#define SERVER_Q_PORT 43000
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

// Signal handler for Ctrl+C
void sigint_handler(int sig) {
    if (sockfd != -1) {
        close(sockfd);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    // Register signal handler
    signal(SIGINT, sigint_handler);
    
    struct sockaddr_in my_addr;     // Server address
    struct sockaddr_in client_addr; // Client address
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int port = SERVER_Q_PORT;

    // Check for custom port argument
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    // Setup server address
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
        perror("bind");
        exit(1);
    }
    
    // Load quotes
    load_quotes_file();
    
    printf("[Server Q] Booting up using UDP on port %d\n", port);
    
    // Main server loop
    while (1) {
        addr_len = sizeof(client_addr);
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr *)&client_addr, &addr_len);
        
        if (bytes_received == -1) {
            perror("recvfrom");
            continue;
        }
        
        buffer[bytes_received] = '\0';
        printf("[Server Q] Received message: %s\n", buffer);
        
        process_message(buffer, &client_addr, addr_len);
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
