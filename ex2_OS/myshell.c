
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

    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
        
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
    // handeling regular commands
    if (pipe_index == -1 && redirect_input == -1 && redirect_output == -1) {
        pid = fork();
        // fork failed
        if (pid < 0) {
            fprintf(stderr, "fork: %s\n", strerror(errno));
            return 0;
        }
        // child process
        else if (pid == 0){
            if (!background){
            // Ignore SIGINT in the child process
            signal(SIGINT, SIG_DFL);
            }
            execvp(arglist[0], arglist);
            fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
            exit(1);
        // parent process
        }else{
            if (!background){
                // wait for the child process to finish
                waitpid(pid, NULL, 0);
            }
            else{
                // don't wait for the child process to finish
                // reap all terminated child processes
                while (waitpid(-1, NULL, WNOHANG) > 0);
            }
        }
    // handeling pipes
    } else if (pipe_index != -1) {
        pid_t pid1, pid2;
        int fd[2];
        if (pipe(fd) == -1) {
            fprintf(stderr, "pipe: %s\n", strerror(errno));
            return 0;
        }
        pid1 = fork();
        // fork failed
        if (pid1 < 0) {
            fprintf(stderr, "fork: %s\n", strerror(errno));
            return 0;
        }
        // first child process
        else if (pid1 == 0){
            // handle sigint
            signal(SIGINT, SIG_DFL);
            // close the read end of the pipe
            close(fd[0]);
            // duplicate the write end of the pipe to stdout
            dup2(fd[1], STDOUT_FILENO);
            // close the write end of the pipe
            close(fd[1]);
            // execute the command
            execvp(arglist[0], arglist);
            fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
            exit(1);
        }
        // parent process
        else if (pid1 > 0){
            pid2 = fork();
            // fork failed
            if (pid2 < 0) {
                fprintf(stderr, "fork: %s\n", strerror(errno));
                return 0;
            }
            // second child process
            else if (pid2 == 0) {
                // handle sigint
                signal(SIGINT, SIG_DFL);    
                // close the write end of the pipe
                close(fd[1]);
                // duplicate the read end of the pipe to stdin
                dup2(fd[0], STDIN_FILENO);
                // close the read end of the pipe
                close(fd[0]);
                // execute the command
                execvp(arglist[pipe_index + 1], arglist + pipe_index + 1);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            }
            // parent process
            else if (pid2 > 0){
                // close the pipe
                close(fd[0]);
                close(fd[1]);
                // wait for the child processes to finish
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
            }
        }
    // input redirection
    } else if (redirect_input != -1){
        int fd;
        pid = fork();
        // fork failed
        if (pid < 0) {
            fprintf(stderr, "fork: %s\n", strerror(errno));
            return 0;
        }
        // child process
        else if (pid == 0){
            // handle sigint
            signal(SIGINT, SIG_DFL);
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
        // parent process
        else if (pid > 0){
            // wait for the child process to finish
            waitpid(pid, NULL, 0);
        }
    
    // output redirection
    } else if (redirect_output != -1){
        
    int fd;
    pid = fork();
        if (pid < 0) {
            fprintf(stderr, "fork: %s\n", strerror(errno));
            return 0;
        }
        // child process
        else if (pid == 0){
            // handle sigint
            signal(SIGINT, SIG_DFL);
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
        else if (pid > 0){
            // parent process
            // wait for the child process to finish
            waitpid(pid, NULL, 0);
        }

    }    
    // if the command is executed successfully the shell will return 1
    return 1;
}


int finalize(void){
    return 0;
}


