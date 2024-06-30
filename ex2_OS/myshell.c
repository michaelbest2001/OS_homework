
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int reg_and_back_command(int background, char **arglist);
int pipe_command(int pipe_index, char **arglist);
int input_redirection(int redirect_input, char **arglist);
int output_redirection(int redirect_output, char **arglist);


int prepare(void){
    // set the signal handler for SIGINT to ignore
    if (signal(SIGINT, SIG_IGN) == SIG_ERR){
        fprintf(stderr, "signal: %s\n", strerror(errno));
        return 1;
    }
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR){
        fprintf(stderr, "signal: %s\n", strerror(errno));
        return 1;
    }
        
    return 0;
}

int process_arglist(int count, char **arglist){

   // pid_t pid;
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
    if (pipe_index == -1 && redirect_input == -1 && redirect_output == -1) {
        return reg_and_back_command(background, arglist);
    } else if (pipe_index != -1) {
        return pipe_command(pipe_index, arglist);
    } else if (redirect_input != -1) {
        return input_redirection(redirect_input, arglist);
    } else if (redirect_output != -1) {
        return output_redirection(redirect_output, arglist);
    }
return 0;
}
// function for regular and background commands
// input: background - 1 if the command is a background command, 0 otherwise
//        arglist - list of arguments
// output: 1 or the command value if successful, 0 otherwise

int reg_and_back_command(int background, char **arglist){
    pid_t pid;
   
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return 0;
    }
    else if (pid == 0){
        if (background){
            // Ignore SIGINT in the child process
            if (signal(SIGINT, SIG_IGN) == SIG_ERR){
                fprintf(stderr, "signal: %s\n", strerror(errno));
                exit(1);
            }
        }
        else{
            // Handle SIGINT in the child process
            if (signal(SIGINT, SIG_DFL) == SIG_ERR){
                fprintf(stderr, "signal: %s\n", strerror(errno));
                exit(1);
            }
        }
        execvp(arglist[0], arglist);
        fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
        exit(1);
    }else{
        if (!background){
            // wait for the child process to finish
            if (waitpid(pid, NULL, 0) == -1){
                if (errno != ECHILD && errno != EINTR){
                    fprintf(stderr, "waitpid: %s\n", strerror(errno));
                    return 0;
                }
            }
        }
    }
    return 1;
}
// function for pipe command
// input: pipe_index - index of the pipe symbol
//        arglist - list of arguments
// output: 1 or the command value if successful, 0 otherwise

int pipe_command(int pipe_index, char **arglist){
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
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){
            fprintf(stderr, "signal: %s\n", strerror(errno));
            exit(1);
        }
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
            if (signal(SIGINT, SIG_DFL) == SIG_ERR){
                fprintf(stderr, "signal: %s\n", strerror(errno));
                exit(1);
            }   
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
            if(waitpid(pid1, NULL, 0) == -1 || waitpid(pid2, NULL, 0) == -1){
                if (errno != ECHILD && errno != EINTR){
                    fprintf(stderr, "waitpid: %s\n", strerror(errno));
                    return 0;
                }
            }     
        }
    }
    return 1;
}

// function for input redirection
// input: redirect_input - index of the input redirection symbol
//        arglist - list of arguments
// output: 1 or the command value if successful, 0 otherwise

int input_redirection(int redirect_input, char **arglist){
    int fd;
    pid_t pid;
    pid = fork();
    // fork failed
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return 0;
    }
    // child process
    else if (pid == 0){
        // handle sigint
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){
            fprintf(stderr, "signal: %s\n", strerror(errno));
            exit(1);
        }
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
        if (waitpid(pid, NULL, 0) == -1){
            if (errno != ECHILD && errno != EINTR){
                fprintf(stderr, "waitpid: %s\n", strerror(errno));
                return 0;
            }
        }
    }
    return 1;
}
// function for output redirection
// input: redirect_output - index of the output redirection symbol
//        arglist - list of arguments
// output: 1 or the command value if successful, 0 otherwise 
int output_redirection(int redirect_output, char **arglist){
    int fd;
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return 0;
    }
    // child process
    else if (pid == 0){
        // handle sigint
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){
            fprintf(stderr, "signal: %s\n", strerror(errno));
            exit(1);
        }
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
    // parent process
    else if (pid > 0){
        // wait for the child process to finish
        if (waitpid(pid, NULL, 0) == -1){
            if (errno != ECHILD && errno != EINTR){
                fprintf(stderr, "waitpid: %s\n", strerror(errno));
                return 0;
            }
        }
    }
    return 1;
}

// Do nothing
int finalize(void){
    return 0;
}


