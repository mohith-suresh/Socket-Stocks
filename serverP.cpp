// serverP.cpp – Portfolio Server for Stock Trading Simulation

// This server:
// – Loads user portfolios from portfolios.txt
// – Manages user stock holdings and transactions
// – Calculates unrealized gains/losses
// – Communicates with Server M via UDP

// Portions of this code inspired  Beej's Guide
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
#include <fstream>  // Added include


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

void sigint_handler(int sig);
void load_portfolios_file();
std::vector<std::string> split_string(const std::string& str, char delimiter);
void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_buy(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_sell(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_check_shares(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);
void handle_portfolio(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len);

// catch ctrl+c , cleanup (beej guide man pages 9.4)
void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    if (sockfd != -1) {
        close(sockfd);
    }
    
    exit(0);
}

int main(int argc, char *argv[]) {
    // shutdown on sigint , be graceful (beej guide man pages 9.4)
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    // Not using SA_RESTART to ensure interrupted system calls fail with EINTR
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        fprintf(stderr, "[Server P] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    
    // setup udp socket , beej guide 6.3
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

    // try bind first , beej guide 6.3
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        // allow reuseaddr , beej guide man pages 9.20
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
    
    
    // Load portfolios
    load_portfolios_file();
    
    printf("[Server P] Booting up using UDP on port %d\n", SERVER_P_PORT);
    // main loop recv/process , beej guide 6.3
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int numbytes;

    while (1) {
        addr_len = sizeof their_addr;
        // recvfrom loop , beej guide 6.3
        if ((numbytes = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            continue;
        }

        buffer[numbytes] = '\0';
        
        
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
    
}

void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string msg(message);
    std::vector<std::string> parts = split_string(msg, ' ');
    
    if (parts.size() < 1) {
        return;
    }
    
    
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
        
        handle_portfolio(parts, client_addr, client_len);
    }
    else if (parts[0] == "N") {
        printf("[Server P] Sale Denied \n");
        fflush(stdout);            
    }
    else {
        
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
    
    printf("[Server P] Received a buy request from the client.\n");
    
    if (user_portfolios.find(username) == user_portfolios.end()) {
        user_portfolios[username] = Portfolio();
    }
    
    Portfolio& portfolio = user_portfolios[username];
    
    if (portfolio.find(stock_name) == portfolio.end()) {
        portfolio[stock_name] = StockHolding(stock_name, num_shares, price);
    } else {
        StockHolding& holding = portfolio[stock_name];
        
        double old_value = holding.shares * holding.avg_price;
        double new_value = num_shares * price;
        int total_shares = holding.shares + num_shares;
        
        holding.avg_price = (old_value + new_value) / total_shares;
        holding.shares = total_shares;
    }
    
    printf("[Server P] Successfully bought %d shares of %s and updated %s's portfolio.\n", num_shares, stock_name.c_str(), username.c_str());
    
    std::string response = "BUY_SUCCESS " + username + " " + stock_name + " " + 
                          std::to_string(num_shares) + " " + std::to_string(price);
    
    // sendto response , beej guide 6.3
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
    
    
    if (user_portfolios.find(username) == user_portfolios.end()) {
        const char* response = "ERROR: User portfolio not found";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    Portfolio& portfolio = user_portfolios[username];
    
    if (portfolio.find(stock_name) == portfolio.end() || 
        portfolio[stock_name].shares < num_shares) {
        printf("[Server P] Stock %s does not have enough shares in %s's portfolio. Unable to sell %d shares of %s.\n", stock_name.c_str(), username.c_str(), num_shares, stock_name.c_str());
        const char* response = "ERROR: Insufficient shares";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    // printf("[Server P] Waiting for sell confirmation via UDP...\n");
    char user_confirmation = 'Y';
    // socklen_t addrlen = client_len;
    // if (recvfrom(sockfd, &user_confirmation, 1, 0,
    //              (struct sockaddr *)client_addr, &addrlen) == -1) {
    //     perror("recvfrom confirmation");
    //     return;
    // }
    // printf("[Server P] Received confirmation byte: '%c' (0x%02x)\n", user_confirmation, (unsigned char)user_confirmation);
    
    if (user_confirmation == 'Y' || user_confirmation == 'y') {
        printf("[Server P] User approves selling the stock.\n");
        StockHolding& holding = portfolio[stock_name];
        holding.shares -= num_shares;
        
        double profit = num_shares * (price - holding.avg_price);
        
        std::string response = "SELL_CONFIRMED: " + std::to_string(num_shares) + 
                              " shares of " + stock_name + " at $" + std::to_string(price) + 
                              ", profit/loss: $" + std::to_string(profit);
        
        printf("[Server P] Successfully sold %d shares of %s and updated %s's portfolio.\n", num_shares, stock_name.c_str(), username.c_str());
        
        // sendto response , beej guide 6.3
        if (sendto(sockfd, response.c_str(), response.length(), 0,
                 (struct sockaddr *)client_addr, client_len) == -1) {
            perror("sendto");
        }
    } else {
        printf("[Server P] Sell denied.\n");
        {
          std::ofstream log("server.logs", std::ios::app);
          log << "[Server P] Sell denied.\n";
        }
        const char* response = "SELL_DENIED";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
    }
}

// void handle_sell(const std::vector<std::string> &p,
//     sockaddr_in *from, socklen_t from_len) {
// const std::string &user  = p[1];
// const std::string &stock = p[2];
// int    shares = std::stoi(p[3]);
// double price  = std::stod(p[4]);

// /* portfolio lookup */
// if (user_portfolios.find(user) == user_portfolios.end() ||
// user_portfolios[user].find(stock) == user_portfolios[user].end() ||
// user_portfolios[user][stock].shares < shares) {
// printf("[Server P] Stock %s does not have enough shares in %s's portfolio. "
//   "Unable to sell %d shares of %s.\n",
//   stock.c_str(), user.c_str(), shares, stock.c_str());
// sendto(sockfd, "INSUFFICIENT_SHARES", 20, 0,
//   (sockaddr *)from, from_len);
// return;
// }

// printf("[Server P] Stock %s has sufficient shares in %s's portfolio. "
// "Requesting users’ confirmation for selling stock.\n",
// stock.c_str(), user.c_str());
// sendto(sockfd, "OK", 3, 0, (sockaddr *)from, from_len);

// /* wait for single-byte Y/N */
// char c;
// if (recvfrom(sockfd, &c, 1, 0,
//     (sockaddr *)from, &from_len) <= 0)
// return;

// if (c == 'Y' || c == 'y') {
// printf("[Server P] User approves selling the stock.\n");
// auto &hold = user_portfolios[user][stock];
// hold.shares -= shares;

// printf("[Server P] Successfully sold %d shares of %s and updated %s's portfolio.\n",
//   shares, stock.c_str(), user.c_str());

// std::string reply = "SOLD " + std::to_string(shares) +
//                " " + stock;
// sendto(sockfd, reply.c_str(), reply.size() + 1, 0,
//   (sockaddr *)from, from_len);
// } else {
// printf("[Server P] Sell denied.\n");
// sendto(sockfd, "SELL_DENIED", 12, 0,
//   (sockaddr *)from, from_len);
// }
// }

void handle_check_shares(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string username = parts[1];
    std::string stock_name = parts[2];
    int num_shares = std::stoi(parts[3]);
    
    
    // Check if user exists
    if (user_portfolios.find(username) == user_portfolios.end()) {
        printf("[Server P] Stock %s does not have enough sharess in %s's portfolio. Unable to sell %d shares of %s.\n", stock_name.c_str(), username.c_str(), num_shares, stock_name.c_str());
        const char* response = "INSUFFICIENT_SHARES";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    Portfolio& portfolio = user_portfolios[username];

    
    printf("[Server P] Received a sell request from the main server.\n");
    
    // Check if user has enough shares
    if (portfolio.find(stock_name) == portfolio.end() || 
        portfolio[stock_name].shares < num_shares) {
            printf("[Server P] Stock %s does not have enough sharessss in %s's portfolio. Unable to sell %d shares of %s.\n", stock_name.c_str(), username.c_str(), num_shares, stock_name.c_str());
        const char* response = "INSUFFICIENT_SHARES";
        sendto(sockfd, response, strlen(response), 0,
             (struct sockaddr *)client_addr, client_len);
        return;
    }
    
    // User has enough shares
    printf("[Server P] Stock %s has sufficient shares in %s's portfolio. Requesting users’ confirmation for selling stock.\n", stock_name.c_str(), username.c_str());

    const char* response = "SUFFICIENT_SHARES";
    sendto(sockfd, response, strlen(response), 0,
         (struct sockaddr *)client_addr, client_len);
}

void handle_portfolio(const std::vector<std::string>& parts, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string username = parts[1];
    
    printf("[Server P] Received a position request from the main server for Member: %s\n", username.c_str());
    
    if (user_portfolios.find(username) == user_portfolios.end()) {
        std::string response = "PORTFOLIO\n";
        sendto(sockfd, response.c_str(), response.length(), 0,
             (struct sockaddr *)client_addr, client_len);
        printf("[Server P] Finished sending the gain and portfolio of %s to the main server.\n", username.c_str());
        return;
    }
    
    Portfolio& portfolio = user_portfolios[username];
    
    std::string response = "PORTFOLIO\n";
    
    for (const auto& holding : portfolio) {
        const StockHolding& stock = holding.second;
        
        if (stock.shares > 0) {
            response += stock.stock_name + " " + 
                      std::to_string(stock.shares) + " " + 
                      std::to_string(stock.avg_price) + "\n";
        }
    }
    
    
    // sendto response , beej guide 6.3
    if (sendto(sockfd, response.c_str(), response.length(), 0,
             (struct sockaddr *)client_addr, client_len) == -1) {
        perror("sendto");
    }
    printf("[Server P] Finished sending the gain and portfolio of %s to the main server.\n", username.c_str());
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
