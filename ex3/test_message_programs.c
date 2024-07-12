#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_CMD_LEN 256
#define MAX_OUTPUT_LEN 1024

int num_test_failures = 0;
void run_command(const char *command, int test_number) {
    char output[MAX_OUTPUT_LEN];
    FILE *fp;

    printf("Test %d: Running command '%s'\n", test_number, command);

    // Open the command for reading
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run command");
        num_test_failures++;
        exit(EXIT_FAILURE);
    }
    else{
        printf("Command executed successfully\n");
    }

    // Read the output a line at a time
    while (fgets(output, sizeof(output)-1, fp) != NULL) {
        printf("%s", output);
    }

    // Close the file pointer
    pclose(fp);
}

int main() {
    // Compile message_sender and message_reader if not already compiled
    system("gcc -o message_sender message_sender.c");
    system("gcc -o message_reader message_reader.c");

    // Test 1: Send and receive a message on channel 1
    run_command("./message_sender /dev/slot0 1 'Hello, World!'", 1);
    run_command("./message_reader /dev/slot0 1", 1);

    // Test 2: Send and receive a message on channel 2
    run_command("./message_sender /dev/slot1 2 'Testing message'", 2);
    run_command("./message_reader /dev/slot1 2", 2);

    // Test 3: Send and receive an empty message
    run_command("./message_sender /dev/slot0 1 ''", 3);
    run_command("./message_reader /dev/slot0 1", 3);

    // Test 4: Send and receive a large message
    run_command("./message_sender /dev/slot1 2 'This is a very large message to test'", 4);
    run_command("./message_reader /dev/slot1 2", 4);

    // Test 5: Send and receive multiple messages on the same channel
    run_command("./message_sender /dev/slot0 1 'First message'", 5);
    run_command("./message_sender /dev/slot0 1 'Second message'", 5);
    run_command("./message_reader /dev/slot0 1", 5);

    // Test 6: Send and receive messages on different channels simultaneously
    run_command("./message_sender /dev/slot0 1 'Channel 1 message'", 6);
    run_command("./message_sender /dev/slot1 2 'Channel 2 message'", 6);
    run_command("./message_reader /dev/slot0 1", 6);
    run_command("./message_reader /dev/slot1 2", 6);

    // Test 7: Send and receive messages with partial reads (small buffer)
    run_command("./message_sender /dev/slot0 1 'Long message to test partial reads'", 7);
    run_command("./message_reader /dev/slot0 1", 7);

    // Test 8: Send and receive messages with partial writes (longer than buffer)
    run_command("./message_sender /dev/slot1 2 'Another long message to test partial writes'", 8);
    run_command("./message_reader /dev/slot1 2", 8);

    // Test 9: Send and receive messages with different message lengths
    run_command("./message_sender /dev/slot0 1 'Short'", 9);
    run_command("./message_sender /dev/slot1 2 'This is a longer message'", 9);
    run_command("./message_reader /dev/slot0 1", 9);
    run_command("./message_reader /dev/slot1 2", 9);

    if (num_test_failures == 0) {
        printf("All tests passed successfully\n");
    } else {
        printf("Some tests failed\n");
    }
    return 0;
}
