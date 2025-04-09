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
#include <fcntl.h>
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
int recv_with_retry(int sockfd, char* buffer, size_t buffer_size);
bool send_with_retry(int sockfd, const char* data, size_t data_length);

// Signal handler for Ctrl+C - Following Beej's Guide Section 9.4
void sigint_handler(int sig) {
    (void)sig;  // Explicitly cast to void to prevent unused parameter warning
    
    printf("\n[Client] Caught SIGINT signal, cleaning up and exiting...\n");
    
    if (sockfd != -1) {
        printf("[Client] Closing socket (fd: %d)...\n", sockfd);
        close(sockfd);
    }
    
    printf("[Client] Cleanup complete, exiting.\n");
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
        fprintf(stderr, "[Client] Failed to register SIGINT handler: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("[Client] Registered signal handler for SIGINT\n");
    
    struct sockaddr_in server_addr;
    struct hostent *he;
    int port = SERVER_PORT;

    // Check for custom port argument
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Create TCP socket - following Beej's Guide pattern
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        fprintf(stderr, "[Client] Failed to create socket: %s\n", strerror(errno));
        exit(1);
    }
    
    // Set socket to non-blocking for better timeout control (Beej's section 7.1)
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get");
        close(sockfd);
        exit(1);
    }
    
    // Don't actually set non-blocking as it would require major code changes
    // This is where we would add: fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    printf("[Client] Socket created successfully (fd: %d)\n", sockfd);

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
    
    // Send credentials to server using our robust helper
    if (!send_with_retry(sockfd, auth_msg.c_str(), auth_msg.length())) {
        return false;
    }
    
    // Get response using our robust helper function
    int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
    
    if (bytes_received <= 0) {
        // Error already reported by recv_with_retry
        return false;
    }
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
    
    // Send command to server using our robust helper
    if (!send_with_retry(sockfd, cmd.c_str(), cmd.length())) {
        return;
    }
    
    // Handle different command types, all using the robust recv helper
    if (parts[0] == "quote") {
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        printf("%s\n", buffer);
    }
    else if (parts[0] == "buy" && parts.size() == 3) {
        // Receive price confirmation
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        
        // Check if error message
        if (strncmp(buffer, "ERROR", 5) == 0) {
            printf("%s\n", buffer);
            return;
        }
        
        printf("%s\n", buffer);
        printf("Confirm buy? (yes/no): ");
        
        std::string confirm;
        std::getline(std::cin, confirm);
        
        // Send confirmation using robust helper
        if (!send_with_retry(sockfd, confirm.c_str(), confirm.length())) {
            return;
        }
        
        // Get final response
        bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        printf("%s\n", buffer);
    }
    else if (parts[0] == "sell" && parts.size() == 3) {
        // Similar to buy but check sufficient shares
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        
        // Check if error message
        if (strncmp(buffer, "ERROR", 5) == 0) {
            printf("%s\n", buffer);
            return;
        }
        
        printf("%s\n", buffer);
        printf("Confirm sell? (yes/no): ");
        
        std::string confirm;
        std::getline(std::cin, confirm);
        
        // Send confirmation using robust helper
        if (!send_with_retry(sockfd, confirm.c_str(), confirm.length())) {
            return;
        }
        
        // Get final response
        bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        printf("%s\n", buffer);
    }
    else if (parts[0] == "position") {
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        printf("%s\n", buffer);
    }
    else {
        // Unknown command or incorrect format
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
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

// Helper function for robust receive with retry logic - as per Beej's Guide Section 7.4
int recv_with_retry(int sockfd, char* buffer, size_t buffer_size) {
    int bytes_received;
    int total_bytes = 0;
    int max_attempts = 5;  // Retry a few times in case of interrupted system calls
    
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        bytes_received = recv(sockfd, buffer + total_bytes, buffer_size - 1 - total_bytes, 0);
        
        if (bytes_received == -1) {
            if (errno == EINTR) {
                // Interrupted system call, retry
                printf("[Client] recv() interrupted, retrying...\n");
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout occurred
                printf("[Client] recv() timed out\n");
                return -1;
            } else {
                // Some other error
                perror("recv");
                return -1;
            }
        } else if (bytes_received == 0) {
            // Server closed connection
            printf("[Client] Server closed connection\n");
            return 0;
        }
        
        total_bytes += bytes_received;
        
        // If we received a terminating character, we're done
        // This depends on your protocol - here we assume messages are null-terminated or newline-terminated
        if (buffer[total_bytes-1] == '\0' || buffer[total_bytes-1] == '\n') {
            break;
        }
    }
    
    buffer[total_bytes] = '\0';  // Ensure null-termination
    return total_bytes;
}

// Helper function for robust send with retry logic - as per Beej's Guide Section 7.4
bool send_with_retry(int sockfd, const char* data, size_t data_length) {
    int bytes_sent;
    int total_bytes = 0;
    int max_attempts = 5;  // Retry a few times in case of interrupted system calls
    
    while (total_bytes < (int)data_length) {
        for (int attempt = 0; attempt < max_attempts; attempt++) {
            bytes_sent = send(sockfd, data + total_bytes, data_length - total_bytes, 0);
            
            if (bytes_sent == -1) {
                if (errno == EINTR) {
                    // Interrupted system call, retry
                    printf("[Client] send() interrupted, retrying...\n");
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Would block, try again later
                    printf("[Client] send() would block, retrying...\n");
                    usleep(100000);  // Sleep for 100ms before retry
                    continue;
                } else {
                    // Some other error
                    perror("send");
                    return false;
                }
            }
            
            total_bytes += bytes_sent;
            break;  // Exit attempt loop on success
        }
        
        // If we still haven't made progress after all attempts
        if (total_bytes < (int)data_length && bytes_sent <= 0) {
            printf("[Client] Failed to send data after multiple attempts\n");
            return false;
        }
    }
    
    return true;
}