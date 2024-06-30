
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


// Reap all terminated child processes

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int prepare(void){

    struct sigaction sa_chld, sa_int;
    
    // Handle SIGCHLD to prevent zombie processes
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        fprintf(stderr, "sigaction(SIGCHLD): %s\n", strerror(errno));
        return -1;
    }

    // Ignore SIGINT in the shell (parent) process
    sa_int.sa_handler = SIG_IGN;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        fprintf(stderr, "sigaction(SIGINT): %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int process_arglist(int count, char **arglist){

    pid_t pid;
    int background = 0;
    int pipe_index = -1;
    int redirect_input = -1;
    int redirect_output = -1;

    

    // Check for special characters
    for (int i = 0; i < count; i++) {
        if (strcmp(arglist[i], "&") == 0) {
            background = 1;
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], "|") == 0) {
            pipe_index = i;
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], "<") == 0) {
            redirect_input = i;
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], ">>") == 0) {
            redirect_output = i;
            arglist[i] = NULL;
        }
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return 0;
    }
    if (pid == 0){
        if (!background){
            // Foreground child processes should terminate upon SIGINT.
            struct sigaction sa_int;
            sa_int.sa_handler = SIG_DFL;
            sigemptyset(&sa_int.sa_mask);
            sa_int.sa_flags = 0;
            if (sigaction(SIGINT, &sa_int, NULL) == -1) {
                fprintf(stderr, "sigaction(SIGINT): %s\n", strerror(errno));
                return 0;
            }
        }
    }
    
    // handeling regular command
    
    if (pipe_index == -1 && redirect_input == -1 && redirect_output == -1) {
        if (pid == 0) {
            // child process

            execvp(arglist[0], arglist);
            fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
            exit(1);
        } else {
            // parent process
            if (!background) {
                waitpid(pid, NULL, 0);
            }
        }

    // handeling pipes
    } else if (pipe_index != -1) {
        /*If arglist contains the word "|" (a single pipe symbol), run two child processes,
         with the output (stdout) of the first process (executing the command that appears before the pipe)
          piped to the input (stdin) of the second process (executing the command that appears after the pipe).
        • To pipe the child processes input and output, use the pipe() and dup2() system calls.
        • Use the same array for all execvp() calls by referencing items in arglist. There’s no need
        to allocate a new array and duplicate parts of the original array.*/
        int fd[2];
        if (pipe(fd) == -1) {
            fprintf(stderr, "pipe: %s\n", strerror(errno));
            return 0;
        }
        if (pid == 0) {
            // child process
            pid_t pid2;
            pid2 = fork();
            if (pid2 < 0) {
                fprintf(stderr, "fork: %s\n", strerror(errno));
                return 0;
            }
            if (pid2 == 0) {
                // second child process
                // handle sigint
                struct sigaction sa_int;
                sa_int.sa_handler = SIG_DFL;
                sigemptyset(&sa_int.sa_mask);
                sa_int.sa_flags = 0;
                if (sigaction(SIGINT, &sa_int, NULL) == -1) {
                    fprintf(stderr, "sigaction(SIGINT): %s\n", strerror(errno));
                    return 0;
                }
                // close the write end of the pipe
                // duplicate the read end of the pipe to stdin
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                // execute the command
                execvp(arglist[pipe_index + 1], arglist + pipe_index + 1);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            } else {
                // first child process
                // close the read end of the pipe
                // duplicate the write end of the pipe to stdout
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                // execute the command
                execvp(arglist[0], arglist);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            }
        } else {
            // parent process
            // close both ends of the pipe
            close(fd[0]);
            close(fd[1]);
            if (!background) {
                waitpid(pid, NULL, 0);
            }
        }
    // input redirection
    } else if (redirect_input != -1){
        int fd;
        if (pid == 0){
            // child process
            // open the file for reading
            fd = open(arglist[redirect_input + 1], O_RDONLY);
            if (fd == -1){
                fprintf(stderr, "open: %s\n", strerror(errno));
                return 0;
            }
            else{
                // duplicate the file descriptor, and close the original
                dup2(fd, STDIN_FILENO);
                close(fd);
                // execute the command
                execvp(arglist[0], arglist);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            }
        }
        else{
            // parent process
            waitpid(pid, NULL, 0);
        }
    // output redirection
    } else if (redirect_output != -1){
 
        int fd;
        if (pid == 0){
            // child process
            // open the file for writing, create it if it doesn't exist, and append to it
            fd = open(arglist[redirect_output + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1){
                fprintf(stderr, "open: %s\n", strerror(errno));
                return 0;
            }
            else{
                // duplicate the file descriptor, and close the original
                dup2(fd, STDOUT_FILENO);
                close(fd);
                // execute the command
                execvp(arglist[0], arglist);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            }
        }
        else{
            // parent process
            waitpid(pid, NULL, 0);
        }
    }
    return 1;

}


int finalize(void){
    return 0;

}


