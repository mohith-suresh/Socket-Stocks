
// serverM.cpp - Main Server for Stock Trading Simulation

//  This main server:
//  - Handles TCP connections from clients
//  - Routes requests to appropriate backend servers via UDP
//  - Manages authentication, trading, and portfolio operations
 
// Portions of this code are inspired on Beej's Guide to Network Programming 
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

// Default values - last 3 digits of my USC ID is 654
#define SERVER_A_PORT 41654
#define SERVER_P_PORT 42654
#define SERVER_Q_PORT 43654
#define SERVER_M_UDP_PORT 44654
#define SERVER_M_TCP_PORT 45654
#define SERVER_IP "127.0.0.1" 
#define BUFFER_SIZE 1024
#define BACKLOG 10

// Global socket file descriptors for cleanup
int tcp_sockfd = -1;
int udp_sockfd = -1;
std::map<int, std::string> client_usernames;

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

void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    printf("\n[Server M] Caught SIGINT signal, cleaning up and exiting...\n");
    

    if (tcp_sockfd != -1) {
        printf("[Server M] Closing TCP socket (fd: %d)...\n", tcp_sockfd);
        close(tcp_sockfd);
    }
    
    if (udp_sockfd != -1) {
        printf("[Server M] Closing UDP socket (fd: %d)...\n", udp_sockfd);
        close(udp_sockfd);
    }
    
    printf("[Server M] Cleanup complete, exiting.\n");
    exit(0);
}

// Password encryption (offset by +3)
void encrypt_password(char* password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (isalpha(password[i])) {
            char base = islower(password[i]) ? 'a' : 'A';
            password[i] = ((password[i] - base + 3) % 26) + base;
        } 
        else if (isdigit(password[i])) {
            password[i] = ((password[i] - '0' + 3) % 10) + '0';
        }
    }
}

int main(int argc, char *argv[]) {
    // sigaction() -  Beej's Guide Section 9.4 (Signal Handling)
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        fprintf(stderr, "[Server M] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Server M] Registered signal handler for SIGINT\n");
    
    // Setting up TCP socket
    // Beej's Guide Sections 5.1 and 5.2
    struct addrinfo tcp_hints, *tcp_servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&tcp_hints, 0, sizeof tcp_hints);
    tcp_hints.ai_family = AF_INET; 
    tcp_hints.ai_socktype = SOCK_STREAM;
    tcp_hints.ai_flags = AI_PASSIVE; 

    if ((rv = getaddrinfo(NULL, std::to_string(SERVER_M_TCP_PORT).c_str(), &tcp_hints, &tcp_servinfo)) != 0) {
        fprintf(stderr, "[Server M] getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Beej's Guide Section 5.2
    for(p = tcp_servinfo; p != NULL; p = p->ai_next) {
        if ((tcp_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("TCP socket");
            continue;
        }

        // Allow port reuse - Beej's Guide Section 7.1 (setsockopt())
        int yes = 1;
        if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt SO_REUSEADDR");
            close(tcp_sockfd);
            continue;
        }

        if (bind(tcp_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(tcp_sockfd);
            perror("TCP bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "[Server M] Failed to bind TCP socket\n");
        exit(1);
    }

    freeaddrinfo(tcp_servinfo);
    
    // Listening for incoming TCP connections - Beej's Guide Section 5.2
    if (listen(tcp_sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    // [Removed TCP socket setup print]

    // Setting up UDP socket using getaddrinfo and bind
    // Based on Beej's Guide Section 5.3 (Datagram Sockets)
    struct addrinfo udp_hints, *udp_servinfo;
    
    memset(&udp_hints, 0, sizeof udp_hints);
    udp_hints.ai_family = AF_INET; 
    udp_hints.ai_socktype = SOCK_DGRAM;
    udp_hints.ai_flags = AI_PASSIVE; 

    if ((rv = getaddrinfo(NULL, std::to_string(SERVER_M_UDP_PORT).c_str(), &udp_hints, &udp_servinfo)) != 0) {
        fprintf(stderr, "[Server M] getaddrinfo: %s\n", gai_strerror(rv));
        close(tcp_sockfd);
        exit(1);
    }

    for(p = udp_servinfo; p != NULL; p = p->ai_next) {
        if ((udp_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("UDP socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt SO_REUSEADDR");
            close(udp_sockfd);
            continue;
        }

        if (bind(udp_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(udp_sockfd);
            perror("UDP bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "[Server M] Failed to bind UDP socket\n");
        close(tcp_sockfd);
        exit(1);
    }

    freeaddrinfo(udp_servinfo);
    
    // [Removed UDP socket setup print]
    
    // Print bootup message after UDP bind succeeds (spec-compliant)
    printf("[Server M] Booting up using UDP on port %d.\n", SERVER_M_UDP_PORT);
    
    // Main server loop , Following Beej's Guide Section 5.2 (A Simple Stream Server)
    struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof their_addr;
    char client_ip[INET_ADDRSTRLEN];
    
    while (1) {
        // Accepting new TCP connection , Based on Beej's Guide Section 5.2
        int client_sockfd = accept(tcp_sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (client_sockfd == -1) {
            perror("accept");
            continue;
        }
        // Convert client IP to string for logging
        inet_ntop(AF_INET, &(((struct sockaddr_in*)&their_addr)->sin_addr),
                  client_ip, INET_ADDRSTRLEN);
        // [Spec: Remove connection notice print]
        // Forking a child process to handle client, Beej's Guide Section 5.2
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            close(client_sockfd);
            continue;
        }
        if (child_pid == 0) {  // Child process
            close(tcp_sockfd);  // Child don't need the listener
            handle_client(client_sockfd);
            close(client_sockfd);
            exit(0);
        }
        // Parent process
        close(client_sockfd);
        while (waitpid(-1, NULL, WNOHANG) > 0) {
        }
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
    
    printf("[Server M] Received username %s and password ****.\n", username.c_str());
    
    // Encrypt password
    char enc_pass[BUFFER_SIZE];
    strncpy(enc_pass, password.c_str(), BUFFER_SIZE - 1);
    enc_pass[BUFFER_SIZE - 1] = '\0';
    encrypt_password(enc_pass);
    
    // Prepare message for Server A
    std::string auth_message = "AUTH " + username + " " + enc_pass;
    // Set up address for Server A
    memset(&server_a_addr, 0, sizeof(server_a_addr));
    server_a_addr.sin_family = AF_INET;
    server_a_addr.sin_port = htons(SERVER_A_PORT);
    server_a_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Sending AUTH request to Server A using UDP
    if (sendto(udp_sockfd, auth_message.c_str(), auth_message.length(), 0,
               (struct sockaddr *)&server_a_addr, sizeof(server_a_addr)) == -1) {
        perror("sendto");
        return false;
    }
    printf("[Server M] Sent the authentication request to Server A\n");
    
    // Receiving AUTH response from Server A using UDP
    //  Beej's Guide Section  5.8
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom");
        return false;
    }
    buffer[bytes_received] = '\0';
    printf("[Server M] Received the response from server A using UDP over %d\n", SERVER_M_UDP_PORT);
    
    // Process Server A response
    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        const char* success_msg = "AUTH_SUCCESS";
        size_t msg_len = strlen(success_msg) + 1;
        char* null_term_response = new char[msg_len];
        strcpy(null_term_response, success_msg);
        int send_res = send(client_sockfd, null_term_response, msg_len, 0);
        printf("[Server M] Sent the response from server A to the client using TCP over port %d.\n", SERVER_M_TCP_PORT);
        delete[] null_term_response;
        return true;
    } else {
        const char* error_msg = "AUTH_FAILED";
        size_t msg_len = strlen(error_msg) + 1;
        char* null_term_response = new char[msg_len];
        strcpy(null_term_response, error_msg);
        int send_res = send(client_sockfd, null_term_response, msg_len, 0);
        printf("[Server M] Sent the response from server A to the client using TCP over port %d.\n", SERVER_M_TCP_PORT);
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
    
    printf("[Server M] Received a quote request from %s%s%s, using TCP over port %d.\n",
           client_usernames[client_sockfd].c_str(),
           stock_name.empty() ? "" : " for stock ",
           stock_name.empty() ? "" : stock_name.c_str(),
           SERVER_M_TCP_PORT);
    
    // Prepare message for Server Q
    std::string quote_message = "QUOTE " + stock_name;
    
    // Set up address , for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send - to Server Q
    if (sendto(udp_sockfd, quote_message.c_str(), quote_message.length(), 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q");
        const char* error_msg = "ERROR: Failed to get quote";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    printf("[Server M] Sent quote request to server Q.\n");
    printf("[Server M] Forwarded the quote request to server Q.\n");
    
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
    printf("[Server M] Received quote response from server Q.\n");
    printf("[Server M] Received the quote response from server Q using UDP over %d\n", SERVER_M_UDP_PORT);
    
    buffer[bytes_received] = '\0';
    
    size_t resp_len = strlen(buffer) + 1; 
    char* null_term_resp = new char[resp_len];
    strcpy(null_term_resp, buffer);
    
    int send_res = send(client_sockfd, null_term_resp, resp_len, 0);
    if (send_res == -1) {
        perror("send quote result to client");
    } else {
        printf("[Server M] Forwarded the quote response to the client.\n");
    }
    delete[] null_term_resp;
}

void handle_buy(int client_sockfd, const std::string& stock_name, int num_shares) {
    if (client_usernames.find(client_sockfd) == client_usernames.end()) {
        const char* error_msg = "ERROR: Not authenticated";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0);
        return;
    }
    
    struct sockaddr_in server_q_addr;
    char buffer[BUFFER_SIZE];
    
    printf("[Server M] Received a buy request from member %s using TCP over port %d.\n",
           client_usernames[client_sockfd].c_str(), SERVER_M_TCP_PORT);
    
    // getting current price from Server Q
    std::string quote_message = "QUOTE " + stock_name;
    
    // Set address for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server Q 
    size_t quote_len = quote_message.length() + 1;
    char* null_term_quote = new char[quote_len];
    strcpy(null_term_quote, quote_message.c_str());
    
    if (sendto(udp_sockfd, null_term_quote, quote_len, 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q");
        const char* error_msg = "ERROR: Failed to get quote for buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0);
        delete[] null_term_quote;
        return;
    }
    printf("[Server M] Sent quote request to server Q.\n");
    delete[] null_term_quote;
    
    // Receive quote -from Server Q
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server Q");
        const char* error_msg = "ERROR: Failed to get quote for buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); 
        return;
    }
    printf("[Server M] Received quote response from server Q.\n");
    
    buffer[bytes_received] = '\0';
    
    // stock doesn't exist or Error
    if (strncmp(buffer, "ERROR", 5) == 0) {
        send(client_sockfd, buffer, strlen(buffer) + 1, 0);
        return;
    }
    
    // Parse price from Server Q response
    std::string response(buffer);
    std::vector<std::string> parts = split_string(response, ' ');
    
    if (parts.size() < 2) {
        const char* error_msg = "ERROR: Invalid quote response";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0);
        return;
    }
    
    double current_price = std::stod(parts[1]);
    double total_cost = current_price * num_shares;
    
    // ask client for confirmation
    std::string confirm_msg = "BUY CONFIRM: " + stock_name + " " + 
                             std::to_string(num_shares) + " shares at $" + 
                             std::to_string(current_price) + " = $" + 
                             std::to_string(total_cost);
    
    // making sure to include the null terminator
    size_t msg_len = confirm_msg.length() + 1; 
    char* null_term_msg = new char[msg_len];
    strcpy(null_term_msg, confirm_msg.c_str());
    
    int send_res = send(client_sockfd, null_term_msg, msg_len, 0);
    if (send_res == -1) {
        perror("send buy confirmation to client");
        delete[] null_term_msg;
        return;
    } else {
        printf("[Server M] Sent the buy confirmation to the client.\n");
    }
    delete[] null_term_msg;
    
    // getting client confirmation
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
    // [Removed debug print of client confirmation]
    
    if (confirmation != "yes" && confirmation != "YES" && confirmation != "y" && confirmation != "Y") {
        const char* cancel_msg = "Buy transaction cancelled";
        send(client_sockfd, cancel_msg, strlen(cancel_msg) + 1, 0); 
        printf("[Server M] Buy denied.\n");
        return;
    }
    printf("[Server M] Buy approved.\n");
    
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
    
    // Send to Server P
    size_t buy_len = buy_message.length() + 1;
    char* null_term_buy = new char[buy_len];
    strcpy(null_term_buy, buy_message.c_str());
    if (sendto(udp_sockfd, null_term_buy, buy_len, 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to process buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); 
        delete[] null_term_buy;
        return;
    }
    printf("[Server M] Forwarded the buy confirmation response to Server P.\n");
    delete[] null_term_buy;
    
    // Receive confirmation from Server P
    from_len = sizeof(from_addr);
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to confirm buy";
        send(client_sockfd, error_msg, strlen(error_msg) + 1, 0); 
        return;
    }
    
    buffer[bytes_received] = '\0';
    // [Removed debug print of buy confirmation from Server P]
    
    // Advance stock price in Server Q
    std::string advance_message = "ADVANCE " + stock_name;
    
    size_t adv_len = advance_message.length() + 1;
    char* null_term_adv = new char[adv_len];
    strcpy(null_term_adv, advance_message.c_str());
    
    if (sendto(udp_sockfd, null_term_adv, adv_len, 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q (advance)");
    } else {
        printf("[Server M] Sent a time forward request for %s.\n", stock_name.c_str());
        // Clear any incoming response
        from_len = sizeof(from_addr);
        recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&from_addr, &from_len);
    }
    delete[] null_term_adv;
    
    // Forward Server P's response to client with null terminator
    size_t resp_len = strlen(buffer) + 1; 
    char* null_term_resp = new char[resp_len];
    strcpy(null_term_resp, buffer);
    
    int send_res_final = send(client_sockfd, null_term_resp, resp_len, 0);
    if (send_res_final == -1) {
        perror("send buy result to client");
    } else {
        printf("[Server M] Forwarded the buy result to the client.\n");
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
    
    printf("[Server M] Received a sell request from member %s using TCP over port %d.\n",
           client_usernames[client_sockfd].c_str(), SERVER_M_TCP_PORT);
    
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
    printf("[Server M] Sent the quote request to server Q.\n");
    
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
    printf("[Server M] Received quote response from server Q.\n");
    
    // buffer[bytes_received] = '\0';
    // printf("[Server M] Received quote response from Server Q: %s\n", buffer);
    
    // // If stock doesn't exist or error
    // if (strncmp(buffer, "ERROR", 5) == 0) {
    //     send(client_sockfd, buffer, bytes_received, 0);
    //     return;
    // }

    buffer[bytes_received] = '\0';
    
    // stock doesn't exist or Error
    if (strncmp(buffer, "ERROR", 5) == 0) {
        send(client_sockfd, buffer, strlen(buffer) + 1, 0);
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
    
    // check if user has enough shares with Server P
    std::string username = client_usernames[client_sockfd];
    std::string check_message = "CHECK " + username + " " + stock_name + " " + std::to_string(num_shares);
    
    // Set up address for Server P
    memset(&server_p_addr, 0, sizeof(server_p_addr));
    server_p_addr.sin_family = AF_INET;
    server_p_addr.sin_port = htons(SERVER_P_PORT);
    server_p_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server P
    if (sendto(udp_sockfd, check_message.c_str(), check_message.length(), 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to check shares";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    printf("[Server M] Forwarded the sell request to server P.\n");
    
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
    
    // If not enough shares
    if (strcmp(buffer, "INSUFFICIENT_SHARES") == 0) {
        const char* error_msg = "ERROR: You do not have enough shares to sell";
        size_t error_len = strlen(error_msg) + 1;
        char* null_term_error = new char[error_len];
        strcpy(null_term_error, error_msg);
        int send_res = send(client_sockfd, null_term_error, error_len, 0);
        delete[] null_term_error;
        return;
    }
    
    // Ask client for confirmation
    double total_value = current_price * num_shares;
    std::string confirm_msg = "SELL CONFIRM: " + stock_name + " " + 
                             std::to_string(num_shares) + " shares at $" + 
                             std::to_string(current_price) + " = $" + 
                             std::to_string(total_value);
    
    size_t msg_len = confirm_msg.length() + 1;
    char* null_term_msg = new char[msg_len];
    strcpy(null_term_msg, confirm_msg.c_str());
    
    int send_res = send(client_sockfd, null_term_msg, msg_len, 0);
    if (send_res == -1) {
        perror("send sell confirmation to client");
    } else {
        printf("[Server M] Forwarded the sell confirmation to the client.\n");
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
    // [Removed debug print of client confirmation]
    
    if (confirmation != "yes" && confirmation != "YES" && confirmation != "y" && confirmation != "Y") {
        const char* cancel_msg = "Sell transaction cancelled";
        size_t msg_len = strlen(cancel_msg) + 1; 
        char* null_term_msg = new char[msg_len];
        strcpy(null_term_msg, cancel_msg);
        // Forward denial to Server P so it can log “Sell denied.”
        sendto(udp_sockfd, "N", 1, 0,
               (struct sockaddr *)&server_p_addr, sizeof(server_p_addr));
        int send_res = send(client_sockfd, null_term_msg, msg_len, 0);
        printf("[Server M] Forwarded the sell confirmation response to Server P.\n");
        delete[] null_term_msg;
        return;
    }
    
    // Process the sell with Server P
    std::string sell_message = "SELL " + username + " " + stock_name + " " + 
                              std::to_string(num_shares) + " " + std::to_string(current_price);
    
    // Send to Server P 
    size_t sell_len = sell_message.length() + 1;
    char* null_term_sell = new char[sell_len];
    strcpy(null_term_sell, sell_message.c_str());
    if (sendto(udp_sockfd, null_term_sell, sell_len, 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to process sell";
        size_t error_len = strlen(error_msg) + 1;
        char* null_term_error = new char[error_len];
        strcpy(null_term_error, error_msg);
        send(client_sockfd, null_term_error, error_len, 0);
        delete[] null_term_error;
        delete[] null_term_sell;
        return;
    }
    printf("[Server M] Forwarded the sell confirmation response to Server P.\n");
    delete[] null_term_sell;
    
    // Receive confirmation from Server P
    from_len = sizeof(from_addr);
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to confirm sell";
        
        size_t error_len = strlen(error_msg) + 1;
        char* null_term_error = new char[error_len];
        strcpy(null_term_error, error_msg);
        
        send(client_sockfd, null_term_error, error_len, 0);
        delete[] null_term_error;
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Advance stock price in Server Q
    std::string advance_message = "ADVANCE " + stock_name;
    
    size_t advance_len = advance_message.length() + 1;
    char* null_term_advance = new char[advance_len];
    strcpy(null_term_advance, advance_message.c_str());
    
    if (sendto(udp_sockfd, null_term_advance, advance_len, 0,
              (struct sockaddr *)&server_q_addr, sizeof(server_q_addr)) == -1) {
        perror("sendto Server Q (advance)");
    } else {
        printf("[Server M] Sent a time forward request for %s.\n", stock_name.c_str());
        // Clear any incoming response
        from_len = sizeof(from_addr);
        recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&from_addr, &from_len);
    }
    delete[] null_term_advance;
    
    // Forward Server P's response to client
    size_t resp_len = strlen(buffer) + 1; 
    char* null_term_resp = new char[resp_len];
    strcpy(null_term_resp, buffer);
    
    int send_res_final = send(client_sockfd, null_term_resp, resp_len, 0);
    if (send_res_final == -1) {
        perror("send sell result to client");
    } else {
        printf("[Server M] Forwarded the sell result to the client.\n");
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
    printf("[Server M] Received a position request from Member to check %s’s gain using TCP over port %d.\n",
           username.c_str(), SERVER_M_TCP_PORT);
    
    // First, get portfolio from Server P
    std::string portfolio_message = "PORTFOLIO " + username;
    
    // Set up address for Server P
    memset(&server_p_addr, 0, sizeof(server_p_addr));
    server_p_addr.sin_family = AF_INET;
    server_p_addr.sin_port = htons(SERVER_P_PORT);
    server_p_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send to Server P
    if (sendto(udp_sockfd, portfolio_message.c_str(), portfolio_message.length(), 0,
              (struct sockaddr *)&server_p_addr, sizeof(server_p_addr)) == -1) {
        perror("sendto Server P");
        const char* error_msg = "ERROR: Failed to get portfolio";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    printf("[Server M] Forwarded the position request to server P.\n");
    
    // Receive portfolio from Server P
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received;
    
    if ((bytes_received = recvfrom(udp_sockfd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len)) == -1) {
        perror("recvfrom Server P");
        const char* error_msg = "ERROR: Failed to get portfolio";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    printf("[Server M] Received user’s portfolio from server P using UDP over %d\n", SERVER_M_UDP_PORT);
    buffer[bytes_received] = '\0';
    std::string portfolio(buffer);
    // Now for each stock in portfolio, get current price from Server Q
    std::vector<std::string> portfolio_lines = split_string(portfolio, '\n');
    if (portfolio_lines.empty()) {
        const char* error_msg = "ERROR: Empty portfolio response";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    if (portfolio_lines[0] != "PORTFOLIO") {
        const char* error_msg = "ERROR: Invalid portfolio response";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Set up address for Server Q
    memset(&server_q_addr, 0, sizeof(server_q_addr));
    server_q_addr.sin_family = AF_INET;
    server_q_addr.sin_port = htons(SERVER_Q_PORT);
    server_q_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    std::string result;
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
        
        // Add to result in required format
        char formatted_line[100];
        snprintf(formatted_line, sizeof(formatted_line), "%s %d %.6f", stock_name.c_str(), shares, avg_price);
        result += std::string(formatted_line) + "\n";
    }
    
    char profit_line[100];
    snprintf(profit_line, sizeof(profit_line), "Total unrealized gain/loss: $%.6f", total_gain);
    result += std::string(profit_line);
    
    // Send result to client
    size_t result_len = result.length() + 1;
    char* null_term_result = new char[result_len];
    strcpy(null_term_result, result.c_str());
    
    int send_res = send(client_sockfd, null_term_result, result_len, 0);
    if (send_res == -1) {
        perror("send portfolio result to client");
    } else {
        printf("[Server M] Forwarded the gain to the client.\n");
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
