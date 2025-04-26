//  serverA.cpp - Authentication Server for Stock Trading Simulation

// This server:
// - Loads user credentials from members.txt
// - Authenticates users by comparing encrypted credentials
// - Communicates with Server M via UDP

// Portions of this code are inspired on Beej guide


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
#define SERVER_A_PORT 41654
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

// handle ctrl+c (beej guide man pages 9.4)
void sigint_handler(int sig) {
    (void)sig;  
    
    // Following Beej's Guide Man Pages 9.4 (close())
    if (sockfd != -1) {
        close(sockfd);
    }
    
    exit(0);
}

int main(int argc, char *argv[]) {
    // register sigint handler (beej guide)
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        fprintf(stderr, "[Server A] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    // udp server setup (beej guide, 6.3)
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // Use my IP

    if ((rv = getaddrinfo(NULL, std::to_string(SERVER_A_PORT).c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "[Server A] getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // try bind to first addr (beej guide, 6.3)
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        // allow sock reuse (beej guide man pages 9.20)
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
        fprintf(stderr, "[Server A] Failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    
    // load users
    load_members_file();
    
    printf("[Server A] Booting up using UDP on port %d\n", SERVER_A_PORT);
    
    // listen for msgs (beej guide, 6.3)
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    int numbytes;

    while (1) {
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            continue;
        }

        buffer[numbytes] = '\0';
        
        // print client addr, port (beej guide)
        struct sockaddr_in* client_addr = (struct sockaddr_in*)&their_addr;
        
        process_message(buffer, client_addr, addr_len);
    }
    
    // cleanup socket (beej guide  9.4)
    // Following Beej's Guide 9.4 (close())
    close(sockfd);
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
            // Store encrypted passwords directly from file
            users[parts[0]] = parts[1];
            count++;
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
        
        // sendto dgram style (beej guide, 6.3)
        size_t response_len = strlen(response) + 1; 
        char* null_term_response = new char[response_len];
        strcpy(null_term_response, response);
        
        int send_result = sendto(sockfd, null_term_response, response_len, 0,
                             (struct sockaddr *)client_addr, client_len);
        if (send_result == -1) {
            perror("sendto");
        }
        
        delete[] null_term_response;
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
