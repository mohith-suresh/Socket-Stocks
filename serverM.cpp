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

// Signal handler for Ctrl+C - Following Beej's Guide Section 9.4
void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    printf("\n[Server M] Caught SIGINT signal, cleaning up and exiting...\n");
    
    // Close all open sockets
    if (tcp_sockfd != -1) {
        printf("[Server M] Closing TCP socket (fd: %d)...\n", tcp_sockfd);
        close(tcp_sockfd);
    }
    if (udp_sockfd != -1) {
        printf("[Server M] Closing UDP socket (fd: %d)...\n", udp_sockfd);
        close(udp_sockfd);
    }
    
    // Close all client connections
    printf("[Server M] Closing %zu client connections...\n", client_usernames.size());
    for (const auto& client : client_usernames) {
        close(client.first);
    }
    
    printf("[Server M] Cleanup complete, exiting.\n");
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
    // Register signal handler with sigaction() - Following Beej's Guide Section 9.4
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    // Not using SA_RESTART to ensure interrupted system calls fail with EINTR
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        fprintf(stderr, "[Server M] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Server M] Registered signal handler for SIGINT\n");
    
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

    // Create TCP socket - Following Beej's Guide Section 5.1
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("TCP socket");
        fprintf(stderr, "[Server M] Failed to create TCP socket: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Server M] TCP socket created with fd %d\n", tcp_sockfd);
    
    // Allow port reuse - Following Beej's Guide Section 7.1
    int yes = 1;
    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        fprintf(stderr, "[Server M] Failed to set SO_REUSEADDR option: %s\n", strerror(errno));
        close(tcp_sockfd);
        exit(1);
    }
    
    // Set receive timeout (optional, as per Beej's recommendation in 7.4)
    struct timeval tv;
    tv.tv_sec = 10;  // 10 second timeout
    tv.tv_usec = 0;
    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("setsockopt SO_RCVTIMEO");
        // Non-fatal, just warn
        fprintf(stderr, "[Server M] Warning: Failed to set SO_RCVTIMEO option: %s\n", strerror(errno));
    }
    
    printf("[Server M] Set socket options successfully\n");
    
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

    // Create UDP socket - Following Beej's Guide Section 5.3
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("UDP socket");
        fprintf(stderr, "[Server M] Failed to create UDP socket: %s\n", strerror(errno));
        close(tcp_sockfd); // Clean up TCP socket before exiting
        exit(1);
    }
    
    printf("[Server M] UDP socket created with fd %d\n", udp_sockfd);
    
    // Set UDP socket timeout as per Beej's Guide Section 7.4
    struct timeval udp_tv;
    udp_tv.tv_sec = 5;  // 5 second timeout for UDP operations
    udp_tv.tv_usec = 0;
    if (setsockopt(udp_sockfd, SOL_SOCKET, SO_RCVTIMEO, &udp_tv, sizeof(udp_tv)) == -1) {
        perror("setsockopt UDP SO_RCVTIMEO");
        // Non-fatal, just warn
        fprintf(stderr, "[Server M] Warning: Failed to set UDP SO_RCVTIMEO option: %s\n", strerror(errno));
    }
    
    // Setup UDP server address
    memset(&udp_my_addr, 0, sizeof(udp_my_addr));
    udp_my_addr.sin_family = AF_INET;
    udp_my_addr.sin_port = htons(udp_port);
    udp_my_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind UDP socket - Following Beej's Guide Section 5.3
    if (bind(udp_sockfd, (struct sockaddr *)&udp_my_addr, sizeof(udp_my_addr)) == -1) {
        perror("UDP bind");
        fprintf(stderr, "[Server M] Failed to bind UDP socket to port %d: %s\n", udp_port, strerror(errno));
        close(tcp_sockfd);
        close(udp_sockfd);
        exit(1);
    }
    
    printf("[Server M] Successfully bound UDP socket to port %d\n", udp_port);

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
    
    printf("[Server M] Received authentication request for user %s with password length %zu\n", 
           username.c_str(), password.length());
    
    // Encrypt password
    char enc_pass[BUFFER_SIZE];
    strncpy(enc_pass, password.c_str(), BUFFER_SIZE - 1);
    enc_pass[BUFFER_SIZE - 1] = '\0';
    encrypt_password(enc_pass);
    
    printf("[Server M] Encrypted password for authentication: %s\n", enc_pass);
    
    // Prepare message for Server A
    std::string auth_message = "AUTH " + username + " " + enc_pass;
    printf("[Server M] Prepared auth message for Server A: %s (length: %zu)\n", 
           auth_message.c_str(), auth_message.length());
    
    // Set up address for Server A
    memset(&server_a_addr, 0, sizeof(server_a_addr));
    server_a_addr.sin_family = AF_INET;
    server_a_addr.sin_port = htons(SERVER_A_PORT);
    server_a_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    printf("[Server M] Server A address set to %s:%d\n", SERVER_IP, SERVER_A_PORT);
    
    // Send to Server A (include null terminator in length)
    printf("[Server M] Sending auth message to Server A...\n");
    // Make sure to include the null terminator
    size_t full_length = auth_message.length() + 1; // +1 for null terminator
    char* null_term_msg = new char[full_length];
    strcpy(null_term_msg, auth_message.c_str());
    
    printf("[Server M] Message with null: '%s' (length: %zu)\n", null_term_msg, full_length);
    
    int send_result = sendto(udp_sockfd, null_term_msg, full_length, 0,
                          (struct sockaddr *)&server_a_addr, sizeof(server_a_addr));
    if (send_result == -1) {
        perror("sendto Server A");
        printf("[Server M] Failed to send auth message to Server A: %s\n", strerror(errno));
        const char* error_msg = "AUTH_FAILED";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        delete[] null_term_msg;
        return false;
    }
    printf("[Server M] Successfully sent %d bytes to Server A\n", send_result);
    delete[] null_term_msg;
    
    // Receive response from Server A
    printf("[Server M] Waiting for response from Server A...\n");
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server A");
        printf("[Server M] Failed to receive response from Server A: %s\n", strerror(errno));
        const char* error_msg = "AUTH_FAILED";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return false;
    }
    
    buffer[bytes_received] = '\0';
    printf("[Server M] Received %d bytes from %s:%d: %s\n", 
           bytes_received, 
           inet_ntoa(from_addr.sin_addr), 
           ntohs(from_addr.sin_port),
           buffer);
    
    // Process Server A's response
    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("[Server M] Authentication successful for user %s\n", username.c_str());
        const char* success_msg = "AUTH_SUCCESS";
        printf("[Server M] Sending AUTH_SUCCESS to client (fd: %d)\n", client_sockfd);
        
        // Make sure to send null-terminated string
        size_t msg_len = strlen(success_msg) + 1; // +1 for null terminator
        char* null_term_response = new char[msg_len];
        strcpy(null_term_response, success_msg);
        
        int send_res = send(client_sockfd, null_term_response, msg_len, 0);
        if (send_res == -1) {
            perror("send to client");
            printf("[Server M] Failed to send AUTH_SUCCESS to client: %s\n", strerror(errno));
            delete[] null_term_response;
            return false;
        }
        printf("[Server M] Successfully sent %d bytes to client\n", send_res);
        delete[] null_term_response;
        return true;
    } else {
        printf("[Server M] Authentication failed for user %s\n", username.c_str());
        const char* error_msg = "AUTH_FAILED";
        printf("[Server M] Sending AUTH_FAILED to client (fd: %d)\n", client_sockfd);
        
        // Make sure to send null-terminated string
        size_t msg_len = strlen(error_msg) + 1; // +1 for null terminator
        char* null_term_response = new char[msg_len];
        strcpy(null_term_response, error_msg);
        
        int send_res = send(client_sockfd, null_term_response, msg_len, 0);
        if (send_res == -1) {
            perror("send to client");
            printf("[Server M] Failed to send AUTH_FAILED to client: %s\n", strerror(errno));
        } else {
            printf("[Server M] Successfully sent %d bytes to client\n", send_res);
        }
        delete[] null_term_response;
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
    
    // Forward response to client with null terminator
    size_t resp_len = strlen(buffer) + 1; // +1 for null terminator
    char* null_term_resp = new char[resp_len];
    strcpy(null_term_resp, buffer);
    
    int send_res = send(client_sockfd, null_term_resp, resp_len, 0);
    if (send_res == -1) {
        perror("send quote result to client");
    } else {
        printf("[Server M] Successfully sent %d bytes of quote data\n", send_res);
    }
    delete[] null_term_resp;
}

void handle_buy(int client_sockfd, const std::string& stock_name, int num_shares) {
    if (client_usernames.find(client_sockfd) == client_usernames.end()) {
        const char* error_msg = "ERROR: Not authenticated";
        // Include null terminator
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0);
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
    
    // Send to Server Q (include null terminator)
    size_t quote_len = quote_message.length() + 1;
    char* null_term_quote = new char[quote_len];
    strcpy(null_term_quote, quote_message.c_str());
    
    if (sendto(udp_sockfd, null_term_quote, quote_len, 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q");
        const char* error_msg = "ERROR: Failed to get quote for buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); // Include null terminator
        delete[] null_term_quote;
        return;
    }
    delete[] null_term_quote;
    
    // Receive quote from Server Q
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server Q");
        const char* error_msg = "ERROR: Failed to get quote for buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); // Include null terminator
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // If stock doesn't exist or error
    if (strncmp(buffer, "ERROR", 5) == 0) {
        send(client_sockfd, buffer, strlen(buffer) + 1, 0); // Include null terminator
        return;
    }
    
    // Parse the price from Server Q's response
    std::string response(buffer);
    std::vector<std::string> parts = split_string(response, ' ');
    
    if (parts.size() < 2) {
        const char* error_msg = "ERROR: Invalid quote response";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); // Include null terminator
        return;
    }
    
    double current_price = std::stod(parts[1]);
    double total_cost = current_price * num_shares;
    
    // Ask client for confirmation
    std::string confirm_msg = "BUY CONFIRM: " + stock_name + " " + 
                             std::to_string(num_shares) + " shares at $" + 
                             std::to_string(current_price) + " = $" + 
                             std::to_string(total_cost);
    
    // Make sure to include the null terminator
    size_t msg_len = confirm_msg.length() + 1; // +1 for null terminator
    char* null_term_msg = new char[msg_len];
    strcpy(null_term_msg, confirm_msg.c_str());
    
    printf("[Server M] Sending buy confirmation to client: '%s'\n", null_term_msg);
    int send_res = send(client_sockfd, null_term_msg, msg_len, 0);
    if (send_res == -1) {
        perror("send buy confirmation to client");
        delete[] null_term_msg;
        return;
    } else {
        printf("[Server M] Successfully sent %d bytes of buy confirmation\n", send_res);
    }
    delete[] null_term_msg;
    
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
    printf("[Server M] Received client confirmation: %s\n", confirmation.c_str());
    
    if (confirmation != "yes" && confirmation != "YES" && confirmation != "y" && confirmation != "Y") {
        const char* cancel_msg = "Buy transaction cancelled";
        send(client_sockfd, cancel_msg, strlen(cancel_msg) + 1, 0); // Include null terminator
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
    
    // Send to Server P (include null terminator)
    size_t buy_len = buy_message.length() + 1;
    char* null_term_buy = new char[buy_len];
    strcpy(null_term_buy, buy_message.c_str());
    
    if (sendto(udp_sockfd, null_term_buy, buy_len, 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to process buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); // Include null terminator
        delete[] null_term_buy;
        return;
    }
    delete[] null_term_buy;
    
    // Receive confirmation from Server P
    from_len = sizeof(from_addr);
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to confirm buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); // Include null terminator
        return;
    }
    
    buffer[bytes_received] = '\0';
    printf("[Server M] Received buy confirmation from Server P: %s\n", buffer);
    
    // Advance stock price in Server Q
    std::string advance_message = "ADVANCE " + stock_name;
    
    // Include null terminator
    size_t adv_len = advance_message.length() + 1;
    char* null_term_adv = new char[adv_len];
    strcpy(null_term_adv, advance_message.c_str());
    
    if (sendto(udp_sockfd, null_term_adv, adv_len, 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q (advance)");
    } else {
        // Clear any incoming response
        from_len = sizeof(from_addr);
        recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&from_addr, &from_len);
    }
    delete[] null_term_adv;
    
    // Forward Server P's response to client with null terminator
    size_t resp_len = strlen(buffer) + 1; // +1 for null terminator
    char* null_term_resp = new char[resp_len];
    strcpy(null_term_resp, buffer);
    
    int send_res_final = send(client_sockfd, null_term_resp, resp_len, 0);
    if (send_res_final == -1) {
        perror("send buy result to client");
    } else {
        printf("[Server M] Successfully sent %d bytes of buy result\n", send_res_final);
    }
    delete[] null_term_resp;
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
    
    // Make sure to include the null terminator
    size_t msg_len = confirm_msg.length() + 1; // +1 for null terminator
    char* null_term_msg = new char[msg_len];
    strcpy(null_term_msg, confirm_msg.c_str());
    
    printf("[Server M] Sending sell confirmation to client: '%s'\n", null_term_msg);
    int send_res = send(client_sockfd, null_term_msg, msg_len, 0);
    if (send_res == -1) {
        perror("send sell confirmation to client");
    } else {
        printf("[Server M] Successfully sent %d bytes of sell confirmation\n", send_res);
    }
    delete[] null_term_msg;
    
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
    
    // Forward Server P's response to client with null terminator
    size_t resp_len = strlen(buffer) + 1; // +1 for null terminator
    char* null_term_resp = new char[resp_len];
    strcpy(null_term_resp, buffer);
    
    int send_res_final = send(client_sockfd, null_term_resp, resp_len, 0);
    if (send_res_final == -1) {
        perror("send sell result to client");
    } else {
        printf("[Server M] Successfully sent %d bytes of sell result\n", send_res_final);
    }
    delete[] null_term_resp;
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
    
    // Send result to client with null terminator
    size_t result_len = result.length() + 1; // +1 for null terminator
    char* null_term_result = new char[result_len];
    strcpy(null_term_result, result.c_str());
    
    int send_res = send(client_sockfd, null_term_result, result_len, 0);
    if (send_res == -1) {
        perror("send portfolio result to client");
    } else {
        printf("[Server M] Successfully sent %d bytes of portfolio data\n", send_res);
    }
    delete[] null_term_result;
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
