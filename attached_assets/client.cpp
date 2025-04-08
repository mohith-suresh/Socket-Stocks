/**
 * client.cpp - Stock Trading Simulation Client
 * EE450 Socket Programming Project
 * 
 * This client provides a command-line interface for:
 * - User authentication
 * - Stock quote retrieval
 * - Buying/selling stocks
 * - Portfolio checking
 * - Logging out
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
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// Default values - replace XXX with your USC ID last 3 digits
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 45000
#define BUFFER_SIZE 1024

// Global socket file descriptor for cleanup on exit
int sockfd = -1;

// Function prototypes
void sigint_handler(int sig);
bool authenticate(int sockfd);
void handle_commands(int sockfd);
void process_command(int sockfd, const std::string& cmd);
std::vector<std::string> split_string(const std::string& str, char delimiter);

// Signal handler for Ctrl+C
void sigint_handler(int sig) {
    if (sockfd != -1) {
        close(sockfd);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    // Register signal handler for clean exit
    signal(SIGINT, sigint_handler);
    
    struct sockaddr_in server_addr;
    struct hostent *he;
    char buffer[BUFFER_SIZE];
    int port = SERVER_PORT;

    // Check for custom port argument
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Get server info
    if ((he = gethostbyname(SERVER_IP)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    // Setup server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);

    // Print connection details
    printf("[Client] Attempting to connect to %s:%d\n", SERVER_IP, port);
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        printf("[Client] Connection failed to %s:%d with sockfd %d\n", SERVER_IP, port, sockfd);
        close(sockfd);
        exit(1);
    }

    // Get local port information
    struct sockaddr_in my_addr;
    socklen_t len = sizeof(my_addr);
    getsockname(sockfd, (struct sockaddr*)&my_addr, &len);
    
    printf("[Client] Connected to Main Server using TCP on port %d\n", ntohs(my_addr.sin_port));

    // User authentication
    if (authenticate(sockfd)) {
        // Main command loop
        handle_commands(sockfd);
    }
    
    // Cleanup
    close(sockfd);
    return 0;
}

bool authenticate(int sockfd) {
    char buffer[BUFFER_SIZE];
    std::string username, password;
    
    printf("[Client] Enter username: ");
    std::getline(std::cin, username);
    
    printf("[Client] Enter password: ");
    std::getline(std::cin, password);
    
    // Create authentication message
    std::string auth_msg = "AUTH " + username + " " + password;
    
    // Send credentials to server
    if (send(sockfd, auth_msg.c_str(), auth_msg.length(), 0) == -1) {
        perror("send");
        return false;
    }
    
    // Get response
    int bytes_received;
    if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
        perror("recv");
        return false;
    }
    
    buffer[bytes_received] = '\0';
    std::string response(buffer);
    
    if (response == "AUTH_SUCCESS") {
        printf("[Client] You have been granted access.\n");
        return true;
    } else {
        printf("[Client] The credentials are incorrect. Please try again.\n");
        return false;
    }
}

void handle_commands(int sockfd) {
    std::string command;
    
    printf("[Client] Available commands:\n");
    printf("  quote - Show all stock prices\n");
    printf("  quote <stock> - Show specific stock price\n");
    printf("  buy <stock> <shares> - Buy shares of a stock\n");
    printf("  sell <stock> <shares> - Sell shares of a stock\n");
    printf("  position - View your current portfolio\n");
    printf("  exit - Logout and exit\n\n");
    
    while (true) {
        printf("> ");
        std::getline(std::cin, command);
        
        if (command == "exit") {
            printf("[Client] Exiting...\n");
            break;
        }
        
        process_command(sockfd, command);
    }
}

void process_command(int sockfd, const std::string& cmd) {
    char buffer[BUFFER_SIZE];
    std::vector<std::string> parts = split_string(cmd, ' ');
    
    if (parts.empty()) {
        return;
    }
    
    // Send command to server
    if (send(sockfd, cmd.c_str(), cmd.length(), 0) == -1) {
        perror("send");
        return;
    }
    
    // Handle different command types
    if (parts[0] == "quote") {
        int bytes_received;
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
    else if (parts[0] == "buy" && parts.size() == 3) {
        // Receive price confirmation
        int bytes_received;
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        
        // Check if error message
        if (strncmp(buffer, "ERROR", 5) == 0) {
            printf("%s\n", buffer);
            return;
        }
        
        printf("%s\n", buffer);
        printf("Confirm buy? (yes/no): ");
        
        std::string confirm;
        std::getline(std::cin, confirm);
        
        // Send confirmation
        if (send(sockfd, confirm.c_str(), confirm.length(), 0) == -1) {
            perror("send");
            return;
        }
        
        // Get final response
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
    else if (parts[0] == "sell" && parts.size() == 3) {
        // Similar to buy but check sufficient shares
        int bytes_received;
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        
        // Check if error message
        if (strncmp(buffer, "ERROR", 5) == 0) {
            printf("%s\n", buffer);
            return;
        }
        
        printf("%s\n", buffer);
        printf("Confirm sell? (yes/no): ");
        
        std::string confirm;
        std::getline(std::cin, confirm);
        
        // Send confirmation
        if (send(sockfd, confirm.c_str(), confirm.length(), 0) == -1) {
            perror("send");
            return;
        }
        
        // Get final response
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
    else if (parts[0] == "position") {
        int bytes_received;
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
    else {
        // Unknown command or incorrect format
        int bytes_received;
        if ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            return;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
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
