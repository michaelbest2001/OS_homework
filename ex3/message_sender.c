/*Implement the program in a file named message_sender.c.
Command line arguments:
• argv[1]: message slot file path.
• argv[2]: the target message channel id. Assume a non-negative integer. • argv[3]: the message to pass.
You should validate that the correct number of command line arguments is passed.
The flow:
1. Open the specified message slot device file.
2. Set the channel id to the id specified on the command line.
3. Write the specified message to the message slot file. Don’t include the terminating null character of the C string as part of the message.
4. Close the device.
5. Exit the program with exit value 0.
If an error occurs in any of the above steps, print an appropriate error message (using strerror() or perror()) and exit the program with exit value 1.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "message_slot.h"

#define MSG_SLOT_CHANNEL _IOW(0, 0, unsigned int)


int main(int argc, char* argv[]) {
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
    if (write(fd, argv[3], strlen(argv[3])) == -1) {
        perror("write");
        close(fd);
        exit(1);
    }
    close(fd);
    exit(0);
}
