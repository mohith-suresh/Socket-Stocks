//  client.cpp - Stock Trading Simulation Client

// This client provides a command line interface for:
// - User authentication
// - Stock quote retrieval
// - Buying/selling stocks
// - Portfolio checking
// - Logging out
 
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
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

//My last 3 digits of USC ID is 654
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 45654
#define BUFFER_SIZE 1024

int sockfd = -1;
std::string current_username;

// funcs we use
void sigint_handler(int sig);
bool authenticate(int sockfd);
void handle_commands(int sockfd);
void process_command(int sockfd, const std::string& cmd);
std::vector<std::string> split_string(const std::string& str, char delimiter);
int recv_with_retry(int sockfd, char* buffer, size_t buffer_size);
bool send_with_retry(int sockfd, const char* data, size_t data_length);

// Handle Ctrl+C.. cleanup before exit
void sigint_handler(int sig) {
    (void)sig; // just ignore warning for unused sig
    printf("\n[Client] Got SIGINT (Ctrl+C), doing cleanup then exit..\n");
    if (sockfd != -1) {
        printf("[Client] Closing socket (fd: %d)...\n", sockfd);
        close(sockfd);
    }
    printf("[Client] Cleanup done, bye!\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    // set up Ctrl+C handler
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
    
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Force IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(SERVER_IP, std::to_string(SERVER_PORT).c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "[Client] getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Try all results, connect to first that works (Beej's Guide 6.2)
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl get");
            close(sockfd);
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "[Client] Failed to connect to server\n");
        exit(1);
    }

    // Get server's IP addr (Beej's Guide 6.2)
    char server_ip[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr),
              server_ip, sizeof server_ip);
    
    printf("[Client] Connected to %s:%d\n", server_ip, SERVER_PORT);

    // Get which local port we got
    struct sockaddr_in my_addr;
    socklen_t len = sizeof(my_addr);
    if (getsockname(sockfd, (struct sockaddr*)&my_addr, &len) == -1) {
        perror("getsockname");
    } else {
        printf("[Client] Using local port %d\n", ntohs(my_addr.sin_port));
    }

    freeaddrinfo(servinfo);

    // login bit
    if (authenticate(sockfd)) {
        // command loop
        handle_commands(sockfd);
    }
    // close up shop (Beej's Guide 5.9)
    close(sockfd);
    return 0;
}

bool authenticate(int sockfd) {
    char buffer[BUFFER_SIZE];
    std::string username, password;
    
    printf("[Client] Enter username: ");
    std::getline(std::cin, username);
    printf("[Client] Username received: %s\n", username.c_str());
    
    printf("[Client] Enter password: ");
    std::getline(std::cin, password);
    printf("[Client] Password received (length: %zu)\n", password.length());
    
    // Make auth msg to send
    std::string auth_msg = "AUTH " + username + " " + password;
    printf("[Client] Sending auth message: AUTH %s ******\n", username.c_str());
    printf("[Client] Auth message length: %zu bytes\n", auth_msg.length());
    
    // Send creds to server (with our retry thing)
    if (!send_with_retry(sockfd, auth_msg.c_str(), auth_msg.length())) {
        printf("[Client] Failed to send authentication message\n");
        return false;
    }
    printf("[Client] Auth message sent successfully, waiting for response...\n");
    
    // Wait for auth reply
    printf("[Client] Trying to get auth response...\n");
    int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
    
    if (bytes_received <= 0) {
        printf("[Client] No valid authentication response received (bytes: %d)\n", bytes_received);
        // Error already reported by recv_with_retry
        return false;
    }
    
    buffer[bytes_received] = '\0'; // Ensure null termination
    std::string response(buffer);
    printf("[Client] Received auth response: %s (bytes: %d)\n", response.c_str(), bytes_received);
    
    if (response == "AUTH_SUCCESS") {
        current_username = username;
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
    
    // Send cmd to server (with retry)
    if (!send_with_retry(sockfd, cmd.c_str(), cmd.length())) {
        return;
    }
    
    // Handle all commands, all use recv_with_retry
    if (parts[0] == "quote") {
        printf("[Client] Sent a quote request to the main server.\n");
        
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        getsockname(sockfd, (struct sockaddr*)&client_addr, &client_len);
        int client_port = ntohs(client_addr.sin_port);
        
        printf("[Client] Received the response from the main server using TCP over port %d.\n", client_port);
        printf("%s\n", buffer);
    }
    else if (parts[0] == "buy" && parts.size() == 3) {
        // get price confirm
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        
        // see if error
        if (strncmp(buffer, "ERROR", 5) == 0) {
            printf("%s\n", buffer);
            return;
        }
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        getsockname(sockfd, (struct sockaddr*)&client_addr, &client_len);
        int client_port = ntohs(client_addr.sin_port);
        
        printf("[Client] Received the response from the main server using TCP over port %d.\n", client_port);
        printf("[Client] %s’s current price is $%.6f. Proceed to buy? (Y/N)\n", parts[1].c_str(), atof(strchr(buffer, '$') + 1));
        
        std::string confirm;
        while (true) {
            std::getline(std::cin, confirm);
            if (confirm == "Y" || confirm == "N") {
                break;
            }
            printf("[Client] Invalid input. Please respond with 'Y' or 'N': ");
        }
        
        // send confirm (Y/N)
        if (!send_with_retry(sockfd, confirm.c_str(), confirm.length())) {
            return;
        }
        
        // get final reply
        bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        printf("[Client] %s successfully bought %d shares of %s.\n", current_username.c_str(), std::stoi(parts[2]), parts[1].c_str());
    }
    else if (parts[0] == "sell" && parts.size() == 3) {
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        
        if (strncmp(buffer, "ERROR", 5) == 0) {
            printf("[Client] Error: %s does not have enough shares of %s to sell. Please try again\n", current_username.c_str(), parts[1].c_str());
            return;
        }
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        getsockname(sockfd, (struct sockaddr*)&client_addr, &client_len);
        int client_port = ntohs(client_addr.sin_port);
        
        printf("[Client] Received the response from the main server using TCP over port %d.\n", client_port);
        printf("[Client] %s’s current price is $%.6f. Proceed to sell? (Y/N)\n", parts[1].c_str(), atof(strchr(buffer, '$') + 1));
        
        std::string confirm;
        while (true) {
            std::getline(std::cin, confirm);
            if (confirm == "Y" || confirm == "N") {
                break;
            }
            printf("[Client] Invalid input. Please respond with 'Y' or 'N': ");
        }
        
        if (!send_with_retry(sockfd, confirm.c_str(), confirm.length())) {
            return;
        }
        
        bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        if (confirm == "Y") {
            printf("[Client] %s successfully sold %d shares of %s.\n", current_username.c_str(), std::stoi(parts[2]), parts[1].c_str());
        } else {
            printf("[Client] Sale cancelled.\n");
        }
    }
    else if (parts[0] == "position") {
        printf("[Client] %s sent a position request to the main server.\n", current_username.c_str());
        
        int bytes_received = recv_with_retry(sockfd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) return;
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        getsockname(sockfd, (struct sockaddr*)&client_addr, &client_len);
        int client_port = ntohs(client_addr.sin_port);
        
        printf("[Client] Received the response from the main server using TCP over port %d.\n", client_port);

        std::istringstream iss(buffer);
        std::string line;
        bool printed_header = false;

        while (std::getline(iss, line)) {
            if (line.find("Total unrealized gain/loss:") != std::string::npos) {
                size_t pos = line.find('$');
                if (pos != std::string::npos) {
                    float profit = std::stof(line.substr(pos + 1));
                    printf("[Client] %s’s current profit is $%.6f\n", current_username.c_str(), profit);
                }
            } else {
                if (!printed_header) {
                    printf("stock shares avg_buy_price\n");
                    printed_header = true;
                }
                printf("%s\n", line.c_str());
            }
        }
    }
    else {
        //  wrong format
        printf("[Client] ERROR: Unknown command or bad format: '%s'\n", cmd.c_str());
        printf("[Client] Try again with a real command.\n");

        // maybe server sent something anyway, let's peek for a sec
        struct timeval tv;
        tv.tv_sec = 1; 
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("[Client] Server response: %s\n", buffer);
        }
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

// recv_with_retry: try to receive, retry a bit if interrupted
int recv_with_retry(int sockfd, char* buffer, size_t buffer_size) {
    int bytes_received;
    int total_bytes = 0;
    int max_attempts = 5;  // try a few times if we get interrupted
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        bytes_received = recv(sockfd, buffer + total_bytes, buffer_size - 1 - total_bytes, 0);
        if (bytes_received == -1) {
            if (errno == EINTR) {
                // got interrupted, try again
                printf("[Client] recv() interrupted, retrying...\n");
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // timed out
                printf("[Client] recv() timed out\n");
                return -1;
            } else {
                // something else broke
                perror("recv");
                return -1;
            }
        } else if (bytes_received == 0) {
            // server closed it
            printf("[Client] Server closed connection\n");
            return 0;
        }
        total_bytes += bytes_received;
        // If got null or newline, done (depends on protocol, but works for us)
        if (buffer[total_bytes-1] == '\0' || buffer[total_bytes-1] == '\n') {
            break;
        }
    }
    buffer[total_bytes] = '\0'; 
    return total_bytes;
}

// send_with_retry: send all data, retry if interrupted (Beej's Guide 7.4)
bool send_with_retry(int sockfd, const char* data, size_t data_length) {
    int bytes_sent;
    int total_bytes = 0;
    int max_attempts = 5;  
    
    char* null_terminated_data = new char[data_length + 1];
    memcpy(null_terminated_data, data, data_length);
    null_terminated_data[data_length] = '\0';
    printf("[Client] Sending data: '%s' (length: %zu + null = %zu bytes)\n",
           null_terminated_data, data_length, data_length + 1);
    // send the null too
    size_t full_length = data_length + 1;
    while (total_bytes < (int)full_length) {
        for (int attempt = 0; attempt < max_attempts; attempt++) {
            bytes_sent = send(sockfd, null_terminated_data + total_bytes, full_length - total_bytes, 0);
            if (bytes_sent == -1) {
                if (errno == EINTR) {
                    // got interrupted, try again
                    printf("[Client] send() interrupted, retrying...\n");
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // would block, try again in a sec
                    printf("[Client] send() would block, retrying...\n");
                    usleep(100000);  // sleep 100ms
                    continue;
                } else {
                    // borked
                    perror("send");
                    delete[] null_terminated_data;
                    return false;
                }
            }
            total_bytes += bytes_sent;
            break; // got some out, break attempt loop
        }
        // if still stuck after all tries
        if (total_bytes < (int)full_length && bytes_sent <= 0) {
            printf("[Client] Failed to send data after multiple attempts\n");
            delete[] null_terminated_data;
            return false;
        }
    }
    delete[] null_terminated_data;
    return true;
}