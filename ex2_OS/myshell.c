#include <cstddef>


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
    // create a child process
    pid = fork();

    // handeling regular commands
    if (pipe_index == -1 && redirect_input == -1 && redirect_output == -1) {
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

    

    } else if (pipe_index != -1) {
        // handeling pipes
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
        else {
            pid1 = fork();
            
        }



    } else if 
    


}

int finalize(void){

    /*2.3 int finalize(void)
The skeleton calls this function before exiting. This function returns 0 on success; any other return value indicates an error.
You can use this function for any cleanups related to process_arglist() that you think are necessary.
 If you don’t need any cleanups, just have this function return immediately; but you must provide it for the skeleton to compile.
  Note that cleaning up the arglist array is not your responsibility. It is taken care of by the skeleton code.*/
}

