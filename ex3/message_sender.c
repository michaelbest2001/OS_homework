
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
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <message slot file path> <target message channel id> <message>\n", argv[0]);
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
    // Write the specified message to the message slot file.
    if (write(fd, argv[3], strlen(argv[3])) == -1) {
        perror("write");
        close(fd);
        exit(1);
    }
    close(fd);
    exit(0);
}
