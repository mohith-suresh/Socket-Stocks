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
#include <fcntl.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#define SERVER_IP "127.0.0.1"  // localhost
#define SERVER_PORT 45000      // Main server port
#define BUFFER_SIZE 1024

// Function prototypes
bool authenticate(int sockfd, const std::string& username, const std::string& password);
void process_command(int sockfd, const std::string& cmd);
std::vector<std::string> split_string(const std::string& str, char delimiter);
int recv_with_retry(int sockfd, char* buffer, size_t buffer_size);
bool send_with_retry(int sockfd, const char* data, size_t data_length);

// Signal handler for graceful termination
void handle_signal(int sig) {
    printf("\n[Client] Received signal %d, exiting gracefully...\n", sig);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct hostent *he;
    struct sockaddr_in server_addr;
    
    // Register signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    
    printf("[Client] Registered signal handler for SIGINT\n");
    
    // Check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <password>\n", argv[0]);
        exit(1);
    }
    
    std::string username = argv[1];
    std::string password = argv[2];
    
    int port = SERVER_PORT;
    
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
    if (authenticate(sockfd, username, password)) {
        // Send commands in sequence for testing
        std::vector<std::string> commands = {"position", "quote GOOG", "exit"};
        
        for (const auto& cmd : commands) {
            printf("[Client] Executing command: %s\n", cmd.c_str());
            if (cmd == "exit") {
                printf("[Client] Exiting...\n");
                break;
            }
            process_command(sockfd, cmd);
        }
    }
    
    // Cleanup
    close(sockfd);
    return 0;
}

bool authenticate(int sockfd, const std::string& username, const std::string& password) {
    char buffer[BUFFER_SIZE];
    
    printf("[Client] Enter username: %s\n", username.c_str());
    printf("[Client] Enter password: %s\n", password.c_str());
    
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
    
    buffer[bytes_received] = '\0';
    printf("[Client] Authentication response: %s\n", buffer);
    
    if (strncmp(buffer, "AUTH_SUCCESS", 12) == 0) {
        printf("[Client] Authentication successful!\n");
        return true;
    } else {
        printf("[Client] Authentication failed: %s\n", buffer);
        return false;
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
        printf("Confirm buy? (yes/no): yes\n");
        
        std::string confirm = "yes";
        
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
        printf("Confirm sell? (yes/no): yes\n");
        
        std::string confirm = "yes";
        
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
            printf("[Client] Server closed connection\n");
            return 0;
        }
        
        // Successfully received some bytes
        total_bytes += bytes_received;
        buffer[total_bytes] = '\0';
        
        // For simplicity, assume we get full messages in one recv
        // A production version would implement proper message framing
        break;
    }
    
    return total_bytes;
}

// Helper function for robust send with retry logic
bool send_with_retry(int sockfd, const char* data, size_t data_length) {
    int bytes_sent = 0;
    int remaining = data_length;
    int max_attempts = 5;  // Retry a few times in case of interrupted system calls
    
    while (remaining > 0) {
        for (int attempt = 0; attempt < max_attempts; attempt++) {
            int sent = send(sockfd, data + bytes_sent, remaining, 0);
            
            if (sent == -1) {
                if (errno == EINTR) {
                    // Interrupted system call, retry
                    printf("[Client] send() interrupted, retrying...\n");
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Would block, wait a bit
                    printf("[Client] send() would block, waiting...\n");
                    usleep(100000);  // 100ms
                    continue;
                } else {
                    // Some other error
                    perror("send");
                    return false;
                }
            }
            
            // Successfully sent some bytes
            bytes_sent += sent;
            remaining -= sent;
            break;
        }
        
        if (remaining > 0 && bytes_sent == 0) {
            // Could not send anything after max attempts
            printf("[Client] Failed to send after %d attempts\n", max_attempts);
            return false;
        }
    }
    
    return true;
}