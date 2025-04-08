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
    int port = SERVER_A_PORT;

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
    
    if (parts.size() < 1) {
        return;
    }
    
    // Handle authentication request
    if (parts[0] == "AUTH" && parts.size() == 3) {
        std::string username = parts[1];
        std::string password = parts[2];
        
        printf("[Server A] Received username %s and password ******.\n", username.c_str());
        
        // Convert username to lowercase for case-insensitive comparison
        std::string lowercase_username = username;
        for (char& c : lowercase_username) {
            c = tolower(c);
        }
        
        // Check if user exists and password matches
        bool authenticated = false;
        
        for (const auto& user : users) {
            std::string stored_username = user.first;
            
            // Convert stored username to lowercase for comparison
            std::string lowercase_stored = stored_username;
            for (char& c : lowercase_stored) {
                c = tolower(c);
            }
            
            if (lowercase_username == lowercase_stored && password == user.second) {
                authenticated = true;
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
            printf("[Server A] The username %s or password ****** is incorrect.\n", username.c_str());
        }
        
        if (sendto(sockfd, response, strlen(response), 0,
                 (struct sockaddr *)client_addr, client_len) == -1) {
            perror("sendto");
        }
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
