
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int prepare(void){
/*
The skeleton calls this function before the first invocation of process_arglist().
This function returns 0 on success; any other return value indicates an error.
You can use this function for any initialization and setup that you think are necessary for your process_arglist().
If you don’t need any initialization, just have this function return immediately; 
but you must provide it for the skeleton to compile.
 */

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
        if (pid == 0) {
            // child process
            execvp(arglist[0], arglist);
            perror("execvp");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            return 1;
        } else {
            // parent process
            if (background == 0) {
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
        int pipefd[2];
        pid_t pid1, pid2;
        char **arglist1 = arglist;
        char **arglist2 = arglist + pipe_index + 1;

        if (pipe(pipefd) == -1) {
            perror("pipe");
            return 1;
        }
        else{
            pid1 = fork();
            if (pid1 < 0){
                perror("fork");
                return 1;
            }
            
            else if (pid1 == 0){
                // 1'st child process
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                execvp(arglist1[0], arglist1);
            }
            else{
                pid2 = fork();
                if (pid2 < 0){
                    perror("fork");
                    return 1;
                }
                else if (pid2 == 0){
                    // 2'nd child process
                    close(pipefd[1]);
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]);
                    execvp(arglist2[0], arglist2);
                }
                else{
                    // parent process
                    close(pipefd[0]);
                    close(pipefd[1]);
                    waitpid(pid1, NULL, 0);
                    waitpid(pid2, NULL, 0);
                }

            }

        }
    // input redirection
    } else if (redirect_input != -1){
        // input redirection
        /*If arglist contains the word “<” (a single redirection symbol),
         open the specified file (that appears after the redirection symbol) and then run the child process, 
         with the input (stdin) redirected from the input file.
        • To redirect the child process’ input, use the dup2() system call.*/
        int fd;
        pid = fork();
        if (pid < 0){
            perror("fork");
            return 1;
        }
        else if (pid == 0){
            // child process
            fd = open(arglist[redirect_input + 1], O_RDONLY);
            if (fd == -1){
                perror("open");
                return 1;
            }
            else{
                dup2(fd, STDIN_FILENO);
                close(fd);
                execvp(arglist[0], arglist);
            }
        }
        else{
            // parent process
            waitpid(pid, NULL, 0);
        }
    // output redirection
    } else if (redirect_output != -1){
        /*• If arglist contains the word “>>” (a redirection symbol), open the specified file (that appears after the redirection symbol) 
        and then run the child process, with the output (stdout) redirected to the output file.
        • To redirect the child process’ output, use the dup2() system call.*/
        int fd;
        pid = fork();
        if (pid < 0){
            perror("fork");
            return 1;
        }
        else if (pid == 0){
            // child process
            fd = open(arglist[redirect_output + 1], O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd == -1){
                perror("open");
                return 1;
            }
            else{
                dup2(fd, STDOUT_FILENO);
                close(fd);
                execvp(arglist[0], arglist);
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

    /*2.3 int finalize(void)
The skeleton calls this function before exiting. This function returns 0 on success; any other return value indicates an error.
You can use this function for any cleanups related to process_arglist() that you think are necessary.
 If you don’t need any cleanups, just have this function return immediately; but you must provide it for the skeleton to compile.
  Note that cleaning up the arglist array is not your responsibility. It is taken care of by the skeleton code.*/
    return 0;

}

