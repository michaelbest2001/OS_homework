#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <errno.h>

#define LISTEN_QUEUE_SIZE 10
#define BUFFER_SIZE 1024

// Global array to count printable characters (32 to 126).
unsigned int pcc_total[95] = {0};
int server_socket;

// a signal handler to handle SIGINT
void print_counts(int sig) {
    // Print the counts of each printable character when SIGINT is received
    for (int i = 0; i < 95; i++) {
        printf("char '%c' : %u times\n", i + 32, pcc_total[i]);
    }
    // Close the server socket and exit the program
    close(server_socket);
    exit(0);
}

// a function to process the client request
// it receives the size of the data (N) and the data from the client
// it counts the number of printable characters in the data and sends the count back to the client
void process_client(int client_socket) {
    uint32_t N = 0;
    uint32_t C = 0;
    unsigned char buffer[BUFFER_SIZE];
    
    // Receive the size of the data (N)
    if (recv(client_socket, &N, sizeof(uint32_t), 0) <= 0) {
        fprintf(stderr, "Failed to receive the size of the data\n");
        close(client_socket);
        return;
    }
    // Convert N to host byte order
    N = ntohl(N);

    while (N > 0) {
        // Receive the data from the client
        ssize_t bytes_received = recv(client_socket, buffer, (N < BUFFER_SIZE) ? N : BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // If the client closes the connection, break the loop
            if (bytes_received == 0) {
                break;
            } else if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {
                // If the connection is reset or timed out, close the client socket
                fprintf(stderr, "TCP error: %s\n", strerror(errno));
                close(client_socket);
                return;
            }
            // If an error occurs while receiving the data, print the error and close the client socket
            fprintf(stderr, "Failed to receive the data\n");
            close(client_socket);
            return;
        }

        for (ssize_t i = 0; i < bytes_received; i++) {
            // Count the number of printable characters in the data
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                pcc_total[buffer[i] - 32]++;
                C++;
            }
        }

        N -= bytes_received;
    }

    C = htonl(C);
    send(client_socket, &C, sizeof(uint32_t), 0);
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    uint16_t port = atoi(argv[1]);
    if (port == 0) {
        fprintf(stderr, "Invalid port number\n");
        exit(1);
    }
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Register the signal handler for SIGINT to print the counts of each printable character
    signal(SIGINT, print_counts);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to create a server socket\n");
        exit(1);
    }
    // Reuse the server socket
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "Failed to set the socket option\n");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to bind the server socket\n");
        close(server_socket);
        exit(1);
    }
    // Listen for incoming connections
    //use listen queue of size 10
    if (listen(server_socket, LISTEN_QUEUE_SIZE) < 0) {
        fprintf(stderr, "Failed to listen for incoming connections\n");
        close(server_socket);
        exit(1);
    }

    while (1) {
        // Accept a new connection and process the client request
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            fprintf(stderr, "Failed to accept a new connection\n");
            continue;
        }
        process_client(client_socket);
    }

    close(server_socket);
    return 0;
}
