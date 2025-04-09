/**
 * serverA.cpp - Authentication Server for Stock Trading Simulation
 * EE450 Socket Programming Project
 * 
 * This server:
 * - Loads user credentials from members.txt
 * - Authenticates users by comparing encrypted credentials
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
#define SERVER_A_PORT 41000
#define BUFFER_SIZE 1024
#define MEMBERS_FILE "members.txt"

// Global socket file descriptor for cleanup
int sockfd = -1;
std::map<std::string, std::string> users; // Maps usernames to encrypted passwords

// Function prototypes
void sigint_handler(int sig);
void load_members_file();
void encrypt_password(char* password);
std::vector<std::string> split_string(const std::string& str, char delimiter);
void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len);

// Signal handler for Ctrl+C - Following Beej's Guide Section 9.4
void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    printf("\n[Server A] Caught SIGINT signal, cleaning up and exiting...\n");
    
    if (sockfd != -1) {
        printf("[Server A] Closing socket (fd: %d)...\n", sockfd);
        close(sockfd);
    }
    
    printf("[Server A] Cleanup complete, exiting.\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    // Register signal handler with sigaction() - Following Beej's Guide Section 9.4
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    // Not using SA_RESTART to ensure interrupted system calls fail with EINTR
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        fprintf(stderr, "[Server A] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Server A] Registered signal handler for SIGINT\n");
    
    struct sockaddr_in my_addr;     // Server address
    struct sockaddr_in client_addr; // Client address
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int port = SERVER_A_PORT;

    // Check for custom port argument
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Create UDP socket - Following Beej's Guide Section 5.3
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        fprintf(stderr, "[Server A] Failed to create UDP socket: %s\n", strerror(errno));
        exit(1);
    }
    
    // Allow port reuse - Following Beej's Guide Section 7.1
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        fprintf(stderr, "[Server A] Failed to set SO_REUSEADDR option: %s\n", strerror(errno));
        close(sockfd);
        exit(1);
    }
    
    // Set receive timeout - Following Beej's Guide Section 7.4
    struct timeval tv;
    tv.tv_sec = 5;  // 5 second timeout
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("setsockopt SO_RCVTIMEO");
        // Non-fatal, just warn
        fprintf(stderr, "[Server A] Warning: Failed to set SO_RCVTIMEO option: %s\n", strerror(errno));
    }
    
    printf("[Server A] Socket options set successfully\n");
    
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
    
    // Load member credentials
    load_members_file();
    
    printf("[Server A] Booting up using UDP on port %d\n", port);
    
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
        printf("[Server A] Received message: %s\n", buffer);
        
        process_message(buffer, &client_addr, addr_len);
    }
    
    return 0;
}

void load_members_file() {
    std::ifstream file(MEMBERS_FILE);
    
    if (!file.is_open()) {
        printf("[Server A] Error: Could not open members file: %s\n", MEMBERS_FILE);
        exit(1);
    }
    
    std::string line;
    int count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        std::vector<std::string> parts = split_string(line, ' ');
        
        if (parts.size() == 2) {
            // Store encrypted passwords
            char password[BUFFER_SIZE];
            strncpy(password, parts[1].c_str(), BUFFER_SIZE - 1);
            password[BUFFER_SIZE - 1] = '\0';
            encrypt_password(password);
            
            users[parts[0]] = password;
            count++;
        }
    }
    
    file.close();
    printf("[Server A] Loaded %d user credentials from %s\n", count, MEMBERS_FILE);
}

void process_message(const char* message, struct sockaddr_in* client_addr, socklen_t client_len) {
    std::string msg(message);
    std::vector<std::string> parts = split_string(msg, ' ');
    
    printf("[Server A] Processing message: '%s' (length: %zu)\n", message, strlen(message));
    printf("[Server A] Received from %s:%d\n", 
           inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    printf("[Server A] Parsed into %zu parts\n", parts.size());
    
    if (parts.size() < 1) {
        printf("[Server A] Warning: Message has no parts, ignoring\n");
        return;
    }
    
    // Handle authentication request
    if (parts[0] == "AUTH" && parts.size() == 3) {
        std::string username = parts[1];
        std::string password = parts[2];
        
        printf("[Server A] Received AUTH request for username '%s' with password '%s'\n", 
               username.c_str(), password.c_str());
        
        // Convert username to lowercase for case-insensitive comparison
        std::string lowercase_username = username;
        for (char& c : lowercase_username) {
            c = tolower(c);
        }
        
        // Check if user exists and password matches
        bool authenticated = false;
        
        printf("[Server A] Checking against %zu stored credentials\n", users.size());
        for (const auto& user : users) {
            std::string stored_username = user.first;
            
            // Convert stored username to lowercase for comparison
            std::string lowercase_stored = stored_username;
            for (char& c : lowercase_stored) {
                c = tolower(c);
            }
            
            printf("[Server A] Checking against user '%s' with stored encrypted password '%s'\n", 
                   stored_username.c_str(), user.second.c_str());
            
            if (lowercase_username == lowercase_stored && password == user.second) {
                authenticated = true;
                printf("[Server A] Match found!\n");
                break;
            }
        }
        
        // Send response
        const char* response;
        if (authenticated) {
            response = "AUTH_SUCCESS";
            printf("[Server A] Member %s has been authenticated.\n", username.c_str());
        } else {
            response = "AUTH_FAILED";
            printf("[Server A] The username %s or password '%s' is incorrect.\n", 
                  username.c_str(), password.c_str());
        }
        
        printf("[Server A] Sending response '%s' to %s:%d\n", 
               response, inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
        
        // Make sure to include the null terminator
        size_t response_len = strlen(response) + 1; // +1 for null terminator
        char* null_term_response = new char[response_len];
        strcpy(null_term_response, response);
        
        printf("[Server A] Message with null: '%s' (length: %zu)\n", null_term_response, response_len);
        
        int send_result = sendto(sockfd, null_term_response, response_len, 0,
                             (struct sockaddr *)client_addr, client_len);
        if (send_result == -1) {
            perror("sendto");
            printf("[Server A] Failed to send response: %s\n", strerror(errno));
        } else {
            printf("[Server A] Successfully sent %d bytes (including null terminator)\n", send_result);
        }
        
        delete[] null_term_response;
    } else {
        printf("[Server A] Received unknown message type: %s (parts: %zu)\n", 
               parts[0].c_str(), parts.size());
    }
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
