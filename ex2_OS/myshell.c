
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/*6. Handling of SIGINT:
• Background on SIGINT (this bullet contains things you should know, not things you need to implement in the assignment): When the user presses Ctrl-C, a SIGINT signal is sent (by the OS) to the shell and all its child processes. The SIGINT signal can also be sent to a specific process using the kill() system call. (There’s also a kill program that invokes
this system call, e.g., kill -INT <pid>.)
• After prepare() finishes, the parent (shell) should not terminate upon SIGINT.
• Foreground child processes (regular commands or parts of a pipe) should terminate upon SIGINT.
• Background child processes should not terminate upon SIGINT.
• NOTE: The program execvp()ed in a child process might change SIGINT handling. This is something the shell has no control over. Therefore, the above two bullets apply only to (1) the behavior before execvp() and (2) if the execvp()ed program doesn’t change SIGINT handling. Most programs don’t change the SIGINT handling they inherent, so these bullets apply to basically any program you are likely to test with (sleep, ls, cat, etc.).
• IMPORTANT: To use some signal-related system calls, C11 code must have a certain macro defined. Be sure to use the compilation command line provided in Section 4 or your code might not compile.
• You may only use the signal system call to set the signal disposition to SIG_IGN or to SIG_DFL. For any other use, particularly setting a signal handler, you must use the sigaction system call.
*/

/*You should prevent zombies and remove them as fast as possible.*/
// Reap all terminated child processes

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int prepare(void){
/*
The skeleton calls this function before the first invocation of process_arglist().
This function returns 0 on success; any other return value indicates an error.
You can use this function for any initialization and setup that you think are necessary for your process_arglist().
If you don’t need any initialization, just have this function return immediately; 
but you must provide it for the skeleton to compile.
 */
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
                // child process
                // handle sigint
                struct sigaction sa_int;
                sa_int.sa_handler = SIG_DFL;
                sigemptyset(&sa_int.sa_mask);
                sa_int.sa_flags = 0;
                if (sigaction(SIGINT, &sa_int, NULL) == -1) {
                    fprintf(stderr, "sigaction(SIGINT): %s\n", strerror(errno));
                    return 0;
                }
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                execvp(arglist[pipe_index + 1], arglist + pipe_index + 1);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            } else {
                // parent process
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                execvp(arglist[0], arglist);
                fprintf(stderr, "Invalid shell command: %s\n", strerror(errno));
                exit(1);
            }
        } else {
            // parent process
            close(fd[0]);
            close(fd[1]);
            if (!background) {
                waitpid(pid, NULL, 0);
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
        if (pid == 0){
            // child process
            fd = open(arglist[redirect_input + 1], O_RDONLY);
            if (fd == -1){
                fprintf(stderr, "open: %s\n", strerror(errno));
                return 0;
            }
            else{
                dup2(fd, STDIN_FILENO);
                close(fd);
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
        /*• Appending redirected output. The user enters one command and output file name separated by the redirection symbol (>>),
         for example: cat foo >> file.txt.
          The shell executes the command so that its standard output is redirected to the output file
           (instead of the default, which is to the user’s terminal). 
           If the specified output file does not exist, it is created. If it exists, 
           the output is appended to the file. The shell waits for the command to complete before accepting another command. 
           By default, stdout and stderr are printed to your terminal. But we can redirect that output to a file using the (>>) 
           operator. The >> file.txt does two things: A) It creates a file named “file” if it does not exist, and B) 
        it appends the new contents to the end of “file”.*/
        int fd;
        if (pid == 0){
            // child process
            fd = open(arglist[redirect_output + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1){
                fprintf(stderr, "open: %s\n", strerror(errno));
                return 0;
            }
            else{
                dup2(fd, STDOUT_FILENO);
                close(fd);
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

    /*2.3 int finalize(void)
The skeleton calls this function before exiting. This function returns 0 on success; any other return value indicates an error.
You can use this function for any cleanups related to process_arglist() that you think are necessary.
 If you don’t need any cleanups, just have this function return immediately; but you must provide it for the skeleton to compile.
  Note that cleaning up the arglist array is not your responsibility. It is taken care of by the skeleton code.*/
    return 0;

}
/*gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c*/

