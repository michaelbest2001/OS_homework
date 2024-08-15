#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <file_path>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    uint16_t port = atoi(argv[2]);
    const char *file_path = argv[3];

    // Open the file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        fprintf(stderr, "Failed to open the file: %s\n", file_path);
        exit(1);
    }
    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        fprintf(stderr, "Failed to create a client socket\n");
        close(file_fd);
        exit(1);
    }
    // set the server address and port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert the server IP address to binary form using inet_pton
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        close(client_socket);
        close(file_fd);
        exit(1);
    }
    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to connect to the server\n");
        close(client_socket);
        close(file_fd);
        exit(1);
    }
    
    // get the file size
    off_t file_size = lseek(file_fd, 0, SEEK_END);
    lseek(file_fd, 0, SEEK_SET);

    // send the file size to the server
    uint32_t N = htonl(file_size);
    send(client_socket, &N, sizeof(uint32_t), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        // send the data to the server
        if (send(client_socket, buffer, bytes_read, 0) != bytes_read) {
            fprintf(stderr, "Failed to send the data to the server\n");
            close(client_socket);
            close(file_fd);
            exit(1);
        }
    }

    uint32_t C;
    // receive the number of printable characters from the server
    if (recv(client_socket, &C, sizeof(uint32_t), 0) <= 0) {
        fprintf(stderr, "Failed to receive the data\n");
        close(client_socket);
        close(file_fd);
        exit(1);
    }
    // convert the number of printable characters to host byte order
    C = ntohl(C);
    printf("# of printable characters: %u\n", C);

    close(client_socket);
    close(file_fd);
    return 0;
}
