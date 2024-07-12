
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "message_slot.h"

int main(int argc, char* argv[]) {
    // Validate the number of command line arguments.
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <message slot file path> <target message channel id>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    unsigned int channel_id = atoi(argv[2]);
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) == -1) {
        perror("ioctl");
        close(fd);
        exit(1);
    }
    // initialize the buffer and read the message from the message slot file
    char buffer[128];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        exit(1);
    }
    // print the message to the standard output
    write(STDOUT_FILENO, buffer, bytes_read);
    close(fd);
    exit(0);
}