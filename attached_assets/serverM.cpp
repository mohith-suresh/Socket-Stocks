/**
 * serverM.cpp - Main Server for Stock Trading Simulation
 * EE450 Socket Programming Project
 * 
 * This main server:
 * - Handles TCP connections from clients
 * - Routes requests to appropriate backend servers via UDP
 * - Manages authentication, trading, and portfolio operations
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
#include <iostream>

// Default values - replace XXX with your USC ID last 3 digits
#define SERVER_A_PORT 41000 
#define SERVER_P_PORT 42000 
#define SERVER_Q_PORT 43000 
#define SERVER_M_UDP_PORT 44000 
#define SERVER_M_TCP_PORT 45000
#define SERVER_IP "127.0.0.1"  // localhost for inter-server communication
#define BUFFER_SIZE 1024
#define BACKLOG 10

// Global socket file descriptors for cleanup
int tcp_sockfd = -1;
int udp_sockfd = -1;
std::map<int, std::string> client_usernames; // Maps client sockets to usernames

// Function prototypes
void sigint_handler(int sig);
void encrypt_password(char* password);
std::vector<std::string> split_string(const std::string& str, char delimiter);
bool handle_authentication(int client_sockfd, const std::string& username, const std::string& password);
void handle_quote(int client_sockfd, const std::string& stock_name);
void handle_buy(int client_sockfd, const std::string& stock_name, int num_shares);
void handle_sell(int client_sockfd, const std::string& stock_name, int num_shares);
void handle_position(int client_sockfd);
void handle_client(int client_sockfd);

// Signal handler for Ctrl+C
void sigint_handler(int sig) {
    if (tcp_sockfd != -1) {
        close(tcp_sockfd);
    }
    if (udp_sockfd != -1) {
        close(udp_sockfd);
    }
    // Close all client connections
    for (const auto& client : client_usernames) {
        close(client.first);
    }
    exit(0);
}

// Password encryption function (offset by +3)
void encrypt_password(char* password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (isalpha(password[i])) {
            char base = islower(password[i]) ? 'a' : 'A';
            password[i] = ((password[i] - base + 3) % 26) + base;
        } 
        else if (isdigit(password[i])) {
            password[i] = ((password[i] - '0' + 3) % 10) + '0';
        }
        // Special characters remain unchanged
    }
}

int main(int argc, char *argv[]) {
    // Register signal handler
    signal(SIGINT, sigint_handler);
    
    struct sockaddr_in tcp_my_addr;    // TCP server address
    struct sockaddr_in udp_my_addr;    // UDP server address
    struct sockaddr_in client_addr;   // Client address
    socklen_t sin_size;
    int tcp_port = SERVER_M_TCP_PORT;
    int udp_port = SERVER_M_UDP_PORT;

    // Check for custom port arguments
    if (argc > 1) {
        tcp_port = atoi(argv[1]);
    }
    if (argc > 2) {
        udp_port = atoi(argv[2]);
    }

    // Create TCP socket
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("TCP socket");
        exit(1);
    }
    
    printf("[Server M] TCP socket created with fd %d\n", tcp_sockfd);
    
    // Allow port reuse
    int yes = 1;
    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        exit(1);
    }
    
    printf("[Server M] Set SO_REUSEADDR option\n");
    
    // Setup TCP server address
    memset(&tcp_my_addr, 0, sizeof(tcp_my_addr));
    tcp_my_addr.sin_family = AF_INET;
    tcp_my_addr.sin_port = htons(tcp_port);
    // Bind to all interfaces (0.0.0.0) to allow connections from other machines/containers
    tcp_my_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind TCP socket
    printf("[Server M] Attempting to bind TCP socket to port %d...\n", tcp_port);
    if (bind(tcp_sockfd, (struct sockaddr *)&tcp_my_addr, sizeof(tcp_my_addr)) == -1) {
        perror("TCP bind");
        printf("[Server M] Failed to bind TCP socket to port %d: %s\n", tcp_port, strerror(errno));
        exit(1);
    }
    printf("[Server M] Successfully bound TCP socket\n");
    
    // Listen on TCP socket
    printf("[Server M] Attempting to listen on TCP socket...\n");
    if (listen(tcp_sockfd, BACKLOG) == -1) {
        perror("listen");
        printf("[Server M] Failed to listen on TCP socket: %s\n", strerror(errno));
        exit(1);
    }
    printf("[Server M] Now listening on TCP port %d\n", tcp_port);

    // Create UDP socket
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("UDP socket");
        exit(1);
    }
    
    // Setup UDP server address
    memset(&udp_my_addr, 0, sizeof(udp_my_addr));
    udp_my_addr.sin_family = AF_INET;
    udp_my_addr.sin_port = htons(udp_port);
    udp_my_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind UDP socket
    if (bind(udp_sockfd, (struct sockaddr *)&udp_my_addr, sizeof(udp_my_addr)) == -1) {
        perror("UDP bind");
        exit(1);
    }

    // Print startup message with more details
    struct sockaddr_in tcp_actual;
    socklen_t tcp_len = sizeof(tcp_actual);
    if (getsockname(tcp_sockfd, (struct sockaddr*)&tcp_actual, &tcp_len) == -1) {
        perror("getsockname");
    } else {
        printf("[Server M] Booting up using TCP on port %d (actual: %d) and UDP on port %d\n", 
               tcp_port, ntohs(tcp_actual.sin_port), udp_port);
        printf("[Server M] TCP socket fd: %d, UDP socket fd: %d\n", tcp_sockfd, udp_sockfd);
        printf("[Server M] TCP IP binding: %s\n", inet_ntoa(tcp_actual.sin_addr));
    }
    
    // Main server loop
    while (1) {
        sin_size = sizeof(client_addr);
        int client_sockfd = accept(tcp_sockfd, (struct sockaddr *)&client_addr, &sin_size);
        
        if (client_sockfd == -1) {
            perror("accept");
            continue;
        }
        
        printf("[Server M] Accepted new client connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Instead of forking, handle directly
        printf("[Server M] Handling client directly (no fork)\n");
        handle_client(client_sockfd);
        close(client_sockfd);
        printf("[Server M] Client connection closed, waiting for new connections\n");
    }
    
    return 0;
}

// Process client commands
void handle_client(int client_sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    while ((bytes_received = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::string message(buffer);
        std::vector<std::string> parts = split_string(message, ' ');
        
        if (parts[0] == "AUTH" && parts.size() == 3) {
            if (handle_authentication(client_sockfd, parts[1], parts[2])) {
                client_usernames[client_sockfd] = parts[1];
            }
        } 
        else if (parts[0] == "quote") {
            std::string stock_name = (parts.size() > 1) ? parts[1] : "";
            handle_quote(client_sockfd, stock_name);
        } 
        else if (parts[0] == "buy" && parts.size() == 3) {
            try {
                int shares = std::stoi(parts[2]);
                handle_buy(client_sockfd, parts[1], shares);
            } catch (const std::invalid_argument&) {
                const char* error_msg = "ERROR: Invalid number of shares";
                send(client_sockfd, error_msg, strlen(error_msg), 0);
            }
        } 
        else if (parts[0] == "sell" && parts.size() == 3) {
            try {
                int shares = std::stoi(parts[2]);
                handle_sell(client_sockfd, parts[1], shares);
            } catch (const std::invalid_argument&) {
                const char* error_msg = "ERROR: Invalid number of shares";
                send(client_sockfd, error_msg, strlen(error_msg), 0);
            }
        } 
        else if (parts[0] == "position") {
            handle_position(client_sockfd);
        } 
        else {
            const char* error_msg = "ERROR: Unknown command or incorrect format";
            send(client_sockfd, error_msg, strlen(error_msg), 0);
        }
    }
    
    // Client disconnected or error
    if (bytes_received == 0) {
        printf("[Server M] Client disconnected\n");
    } else if (bytes_received == -1) {
        perror("recv");
    }
    
    // Remove from client map
    client_usernames.erase(client_sockfd);
}

bool handle_authentication(int client_sockfd, const std::string& username, const std::string& password) {
    struct sockaddr_in server_a_addr;
    char buffer[BUFFER_SIZE];
    
    printf("[Server M] Received authentication request for user %s\n", username.c_str());
    
    // Encrypt password
    char enc_pass[BUFFER_SIZE];
    strncpy(enc_pass, password.c_str(), BUFFER_SIZE - 1);
    enc_pass[BUFFER_SIZE - 1] = '\0';
    encrypt_password(enc_pass);
    
    printf("[Server M] Encrypted password for authentication\n");
    
    // Prepare message for Server A
    std::string auth_message = "AUTH " + username + " " + enc_pass;
    
    // Set up address for Server A
    memset(&server_a_addr, 0, sizeof(server_a_addr));
    server_a_addr.sin_family = AF_INET;
    server_a_addr.sin_port = htons(SERVER_A_PORT);
    server_a_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server A
    if (sendto(udp_sockfd, auth_message.c_str(), auth_message.length(), 0,
              (struct sockaddr *)&server_a_addr, sizeof(server_a_addr)) == -1) {
        perror("sendto Server A");
        const char* error_msg = "AUTH_FAILED";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return false;
    }
    
    // Receive response from Server A
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server A");
        const char* error_msg = "AUTH_FAILED";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return false;
    }
    
    buffer[bytes_received] = '\0';
    
    // Process Server A's response
    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("[Server M] Authentication successful for user %s\n", username.c_str());
        const char* success_msg = "AUTH_SUCCESS";
        send(client_sockfd, success_msg, strlen(success_msg), 0);
        return true;
    } else {
        printf("[Server M] Authentication failed for user %s\n", username.c_str());
        const char* error_msg = "AUTH_FAILED";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return false;
    }
}

void handle_quote(int client_sockfd, const std::string& stock_name) {
    if (client_usernames.find(client_sockfd) == client_usernames.end()) {
        const char* error_msg = "ERROR: Not authenticated";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    struct sockaddr_in server_q_addr;
    char buffer[BUFFER_SIZE];
    
    printf("[Server M] Received quote request for %s\n", 
           stock_name.empty() ? "all stocks" : stock_name.c_str());
    
    // Prepare message for Server Q
    std::string quote_message = "QUOTE " + stock_name;
    
    // Set up address for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server Q
    if (sendto(udp_sockfd, quote_message.c_str(), quote_message.length(), 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q");
        const char* error_msg = "ERROR: Failed to get quote";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive response from Server Q
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server Q");
        const char* error_msg = "ERROR: Failed to get quote";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Forward response to client
    send(client_sockfd, buffer, bytes_received, 0);
}

void handle_buy(int client_sockfd, const std::string& stock_name, int num_shares) {
    if (client_usernames.find(client_sockfd) == client_usernames.end()) {
        const char* error_msg = "ERROR: Not authenticated";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    struct sockaddr_in server_q_addr;
    char buffer[BUFFER_SIZE];
    
    printf("[Server M] Received buy request: %s %d shares\n", stock_name.c_str(), num_shares);
    
    // First, get current price from Server Q
    std::string quote_message = "QUOTE " + stock_name;
    
    // Set up address for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server Q
    if (sendto(udp_sockfd, quote_message.c_str(), quote_message.length(), 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q");
        const char* error_msg = "ERROR: Failed to get quote for buy";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive quote from Server Q
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server Q");
        const char* error_msg = "ERROR: Failed to get quote for buy";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // If stock doesn't exist or error
    if (strncmp(buffer, "ERROR", 5) == 0) {
        send(client_sockfd, buffer, bytes_received, 0);
        return;
    }
    
    // Parse the price from Server Q's response
    std::string response(buffer);
    std::vector<std::string> parts = split_string(response, ' ');
    
    if (parts.size() < 2) {
        const char* error_msg = "ERROR: Invalid quote response";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    double current_price = std::stod(parts[1]);
    double total_cost = current_price * num_shares;
    
    // Ask client for confirmation
    std::string confirm_msg = "BUY CONFIRM: " + stock_name + " " + 
                             std::to_string(num_shares) + " shares at $" + 
                             std::to_string(current_price) + " = $" + 
                             std::to_string(total_cost);
    
    send(client_sockfd, confirm_msg.c_str(), confirm_msg.length(), 0);
    
    // Get client confirmation
    if ((bytes_received = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0)) <= 0) {
        if (bytes_received == 0) {
            printf("[Server M] Client disconnected during buy confirmation\n");
        } else {
            perror("recv confirmation");
        }
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string confirmation(buffer);
    
    if (confirmation != "yes" && confirmation != "YES" && confirmation != "y" && confirmation != "Y") {
        const char* cancel_msg = "Buy transaction cancelled";
        send(client_sockfd, cancel_msg, strlen(cancel_msg), 0);
        return;
    }
    
    // Process the buy with Server P
    std::string username = client_usernames[client_sockfd];
    std::string buy_message = "BUY " + username + " " + stock_name + " " + 
                             std::to_string(num_shares) + " " + std::to_string(current_price);
    
    struct sockaddr_in server_p_addr;
    
    // Set up address for Server P
    memset(&server_p_addr, 0, sizeof(server_p_addr));
    server_p_addr.sin_family = AF_INET;
    server_p_addr.sin_port = htons(SERVER_P_PORT);
    server_p_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    printf("[Server M] Sending buy request to Server P: '%s'\n", buy_message.c_str());
    
    // Send to Server P
    if (sendto(udp_sockfd, buy_message.c_str(), buy_message.length(), 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to process buy";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive confirmation from Server P
    from_len = sizeof(from_addr);
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to confirm buy";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    buffer[bytes_received] = '\0';
    printf("[Server M] Received buy confirmation from Server P: %s\n", buffer);
    
    // Advance stock price in Server Q
    std::string advance_message = "ADVANCE " + stock_name;
    
    if (sendto(udp_sockfd, advance_message.c_str(), advance_message.length(), 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q (advance)");
    } else {
        // Clear any incoming response
        from_len = sizeof(from_addr);
        recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&from_addr, &from_len);
    }
    
    // Forward Server P's response to client
    send(client_sockfd, buffer, strlen(buffer), 0);
}

void handle_sell(int client_sockfd, const std::string& stock_name, int num_shares) {
    if (client_usernames.find(client_sockfd) == client_usernames.end()) {
        const char* error_msg = "ERROR: Not authenticated";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    struct sockaddr_in server_q_addr, server_p_addr;
    char buffer[BUFFER_SIZE];
    
    printf("[Server M] Received sell request: %s %d shares\n", stock_name.c_str(), num_shares);
    
    // First, get current price from Server Q
    std::string quote_message = "QUOTE " + stock_name;
    
    // Set up address for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server Q
    if (sendto(udp_sockfd, quote_message.c_str(), quote_message.length(), 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q");
        const char* error_msg = "ERROR: Failed to get quote for sell";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive quote from Server Q
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server Q");
        const char* error_msg = "ERROR: Failed to get quote for sell";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    buffer[bytes_received] = '\0';
    printf("[Server M] Received quote response from Server Q: %s\n", buffer);
    
    // If stock doesn't exist or error
    if (strncmp(buffer, "ERROR", 5) == 0) {
        send(client_sockfd, buffer, bytes_received, 0);
        return;
    }
    
    // Parse the price from Server Q's response
    std::string response(buffer);
    std::vector<std::string> parts = split_string(response, ' ');
    
    if (parts.size() < 2) {
        const char* error_msg = "ERROR: Invalid quote response";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    double current_price = std::stod(parts[1]);
    printf("[Server M] Parsed current price for %s: $%.2f\n", stock_name.c_str(), current_price);
    
    // Now check if user has enough shares with Server P
    std::string username = client_usernames[client_sockfd];
    std::string check_message = "CHECK " + username + " " + stock_name + " " + std::to_string(num_shares);
    
    // Set up address for Server P
    memset(&server_p_addr, 0, sizeof(server_p_addr));
    server_p_addr.sin_family = AF_INET;
    server_p_addr.sin_port = htons(SERVER_P_PORT);
    server_p_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    printf("[Server M] Sending share check request to Server P: '%s'\n", check_message.c_str());
    
    // Send to Server P
    if (sendto(udp_sockfd, check_message.c_str(), check_message.length(), 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to check shares";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive response from Server P
    from_len = sizeof(from_addr);
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to check shares";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    buffer[bytes_received] = '\0';
    printf("[Server M] Received share check response from Server P: %s\n", buffer);
    
    // If not enough shares
    if (strcmp(buffer, "INSUFFICIENT_SHARES") == 0) {
        const char* error_msg = "ERROR: You do not have enough shares to sell";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Ask client for confirmation
    double total_value = current_price * num_shares;
    std::string confirm_msg = "SELL CONFIRM: " + stock_name + " " + 
                             std::to_string(num_shares) + " shares at $" + 
                             std::to_string(current_price) + " = $" + 
                             std::to_string(total_value);
    
    printf("[Server M] Sending sell confirmation to client: %s\n", confirm_msg.c_str());
    send(client_sockfd, confirm_msg.c_str(), confirm_msg.length(), 0);
    
    // Get client confirmation
    if ((bytes_received = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0)) <= 0) {
        if (bytes_received == 0) {
            printf("[Server M] Client disconnected during sell confirmation\n");
        } else {
            perror("recv confirmation");
        }
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string confirmation(buffer);
    printf("[Server M] Received client confirmation: %s\n", confirmation.c_str());
    
    if (confirmation != "yes" && confirmation != "YES" && confirmation != "y" && confirmation != "Y") {
        const char* cancel_msg = "Sell transaction cancelled";
        send(client_sockfd, cancel_msg, strlen(cancel_msg), 0);
        return;
    }
    
    // Process the sell with Server P
    std::string sell_message = "SELL " + username + " " + stock_name + " " + 
                              std::to_string(num_shares) + " " + std::to_string(current_price);
    
    printf("[Server M] Sending sell request to Server P: '%s'\n", sell_message.c_str());
    
    // Send to Server P
    if (sendto(udp_sockfd, sell_message.c_str(), sell_message.length(), 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to process sell";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive confirmation from Server P
    from_len = sizeof(from_addr);
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to confirm sell";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    buffer[bytes_received] = '\0';
    printf("[Server M] Received sell confirmation from Server P: %s\n", buffer);
    
    // Advance stock price in Server Q
    std::string advance_message = "ADVANCE " + stock_name;
    
    if (sendto(udp_sockfd, advance_message.c_str(), advance_message.length(), 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q (advance)");
    } else {
        // Clear any incoming response
        from_len = sizeof(from_addr);
        recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&from_addr, &from_len);
    }
    
    // Forward Server P's response to client
    send(client_sockfd, buffer, strlen(buffer), 0);
}

void handle_position(int client_sockfd) {
    if (client_usernames.find(client_sockfd) == client_usernames.end()) {
        const char* error_msg = "ERROR: Not authenticated";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    struct sockaddr_in server_p_addr, server_q_addr;
    char buffer[BUFFER_SIZE];
    
    std::string username = client_usernames[client_sockfd];
    printf("[Server M] Received position request from %s\n", username.c_str());
    
    // First, get portfolio from Server P
    std::string portfolio_message = "PORTFOLIO " + username;
    
    // Set up address for Server P
    memset(&server_p_addr, 0, sizeof(server_p_addr));
    server_p_addr.sin_family = AF_INET;
    server_p_addr.sin_port = htons(SERVER_P_PORT);
    server_p_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server P
    printf("[Server M] Sending portfolio request to Server P: '%s'\n", portfolio_message.c_str());
    if (sendto(udp_sockfd, portfolio_message.c_str(), portfolio_message.length(), 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to get portfolio";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Receive portfolio from Server P
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    printf("[Server M] Waiting for portfolio response from Server P...\n");
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to get portfolio";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    printf("[Server M] Received %d bytes from Server P\n", bytes_received);
    
    buffer[bytes_received] = '\0';
    std::string portfolio(buffer);
    
    printf("[Server M] Received portfolio response from Server P: \n%s\n", portfolio.c_str());
    
    // Now for each stock in portfolio, get current price from Server Q
    std::vector<std::string> portfolio_lines = split_string(portfolio, '\n');
    
    printf("[Server M] Split portfolio response into %zu lines\n", portfolio_lines.size());
    
    if (portfolio_lines.empty()) {
        printf("[Server M] Error: Portfolio response is empty\n");
        const char* error_msg = "ERROR: Empty portfolio response";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    if (portfolio_lines[0] != "PORTFOLIO") {
        printf("[Server M] Error: First line is not PORTFOLIO, got: %s\n", portfolio_lines[0].c_str());
        const char* error_msg = "ERROR: Invalid portfolio response";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Set up address for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    std::string result = "Your portfolio:\n";
    double total_gain = 0.0;
    
    for (size_t i = 1; i < portfolio_lines.size(); i++) {
        std::vector<std::string> stock_info = split_string(portfolio_lines[i], ' ');
        
        if (stock_info.size() != 3) {
            continue;
        }
        
        std::string stock_name = stock_info[0];
        int shares = std::stoi(stock_info[1]);
        double avg_price = std::stod(stock_info[2]);
        
        // Skip if no shares
        if (shares == 0) {
            continue;
        }
        
        // Get current price from Server Q
        std::string quote_message = "QUOTE " + stock_name;
        
        if (sendto(udp_sockfd, quote_message.c_str(), quote_message.length(), 0,
                  (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
            perror("sendto Server Q");
            continue;
        }
        
        from_len = sizeof(from_addr);
        
        if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&from_addr, &from_len)) == -1) {
            perror("recvfrom Server Q");
            continue;
        }
        
        buffer[bytes_received] = '\0';
        std::string quote_response(buffer);
        
        std::vector<std::string> quote_parts = split_string(quote_response, ' ');
        
        if (quote_parts.size() < 2 || quote_parts[0] != stock_name) {
            continue;
        }
        
        double current_price = std::stod(quote_parts[1]);
        double stock_gain = shares * (current_price - avg_price);
        total_gain += stock_gain;
        
        // Add to result
        result += stock_name + ": " + std::to_string(shares) + " shares, avg price: $" + 
                 std::to_string(avg_price) + ", current price: $" + std::to_string(current_price) + 
                 ", gain/loss: $" + std::to_string(stock_gain) + "\n";
    }
    
    result += "Total unrealized gain/loss: $" + std::to_string(total_gain);
    
    // Send result to client
    send(client_sockfd, result.c_str(), result.length(), 0);
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
