//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
    printf("****  Exiting MSH **** \n");
    //signal(SIGINT, siginthandler);
    exit(0);
}

/* myhistory */

/* myhistory */

struct command
{
    // Store the number of commands in argvv
    int num_commands;
    // Store the number of arguments of each command
    int *args;
    // Store the commands
    char ***argvv;
    // Store the I/O redirection
    char filev[3][64];
    // Store if the command is executed in background or foreground
    int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1;
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}


/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
    //reset first
    for(int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for ( i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}

// New SIGCHLD handler to avoid zombie processes
void sigchld_handler(int sig) {
    // Use waitpid in a loop to reap all exited children
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

/**
 * Main sheell  Loop
 */
int main(int argc, char* argv[])
{
    /**** Do not delete this code.****/
    int end = 0;
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF){
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush (stdin);
            fflush(stdout);
        }
    }

    // Register SIGCHLD handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    /*********************************/

    char ***argvv = NULL;
    int num_commands;

    history = (struct command*) malloc(history_size *sizeof(struct command));
    int run_history = 0;

    while (1)
    {
        int status = 0;
        int command_counter = 0;
        int in_background = 0;
        signal(SIGINT, siginthandler);

        if (run_history)
        {
            run_history=0;
        }
        else{
            // Prompt
            write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

            // Get command
            //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
            executed_cmd_lines++;
            if( end != 0 && executed_cmd_lines < end) {
                command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
            }
            else if( end != 0 && executed_cmd_lines == end)
                return 0;
            else
                command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
        }
        //************************************************************************************************


        /************************ STUDENTS CODE ********************************/
        if (command_counter > 0) {
            if (command_counter > MAX_COMMANDS) {
                printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
            } else {
                pid_t manager_pid = 0;
                if (in_background) {
                    manager_pid = fork(); // Fork a manager process for background tasks
                }

                if (manager_pid == 0 || !in_background) { // Proceed if it's a foreground task or inside the manager process
                    int pipe_fds[2 * (command_counter - 1)]; // Array to hold the file descriptors for all pipes

                    // Create all needed pipes
                    for (int i = 0; i < command_counter - 1; i++) {
                        if (pipe(pipe_fds + i * 2) < 0) {
                            perror("Couldn't Pipe");
                            exit(EXIT_FAILURE);
                        }
                    }

                    for (int i = 0; i < command_counter; i++) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            // Input redirection for the first command
                            if (i == 0 && strcmp(filev[0], "0") != 0) {
                                int in_fd = open(filev[0], O_RDONLY);
                                if (in_fd < 0) {
                                    perror("Failed to open input file");
                                    exit(EXIT_FAILURE);
                                }
                                dup2(in_fd, STDIN_FILENO);
                                close(in_fd);
                            }

                            // Output redirection for the last command
                            if (i == command_counter - 1 && strcmp(filev[1], "0") != 0) {
                                int out_fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                                if (out_fd < 0) {
                                    perror("Failed to open output file");
                                    exit(EXIT_FAILURE);
                                }
                                dup2(out_fd, STDOUT_FILENO);
                                close(out_fd);
                            }

                            // Error redirection for all commands
                            if (strcmp(filev[2], "0") != 0) {
                                int err_fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                                if (err_fd < 0) {
                                    perror("Failed to open error file");
                                    exit(EXIT_FAILURE);
                                }
                                dup2(err_fd, STDERR_FILENO);
                                close(err_fd);
                            }

                            // Pipe setup for all commands except the first
                            if (i != 0) {
                                if (dup2(pipe_fds[(i - 1) * 2], STDIN_FILENO) < 0) {
                                    perror("dup2");
                                    exit(EXIT_FAILURE);
                                }
                            }

                            // Pipe setup for all commands except the last
                            if (i != command_counter - 1) {
                                if (dup2(pipe_fds[i * 2 + 1], STDOUT_FILENO) < 0) {
                                    perror("dup2");
                                    exit(EXIT_FAILURE);
                                }
                            }

                            // Close all pipe file descriptors
                            for (int j = 0; j < 2 * (command_counter - 1); j++) {
                                close(pipe_fds[j]);
                            }

                            // Execute the command
                            getCompleteCommand(argvv, i); // Prepare the command for execvp
                            if (execvp(argv_execvp[0], argv_execvp) < 0) {
                                perror("execvp");
                                exit(EXIT_FAILURE);
                            }
                        } else if (pid < 0) {
                            perror("error");
                            exit(EXIT_FAILURE);
                        } else {
                            if (i < command_counter - 1) {
                                close(pipe_fds[i * 2 + 1]); // Close write end of the current pipe
                            }
                            if (i > 0) {
                                close(pipe_fds[(i - 1) * 2]); // Close read end of the previous pipe
                            }

                            if (!in_background) {
                                int status;
                                waitpid(pid, &status, 0); // Wait for each command to finish in foreground tasks
                            }
                        }
                    }


                    // Close all pipe file descriptors in the parent
                    for (int i = 0; i < 2 * (command_counter - 1); i++) {
                        close(pipe_fds[i]);
                    }
                    if (in_background) {
                        exit(0); // Exit the manager process to prevent it from continuing the main loop
                    }

                } else if (manager_pid > 0 && in_background) {
                    // Main shell process for background tasks
                    printf("[%d] %d\n", 1, manager_pid);  // Simplified background job notification
                    fflush(stdout);  // Flush the output to ensure the shell prompt is displayed immediately
                }
            }
        }
    }
    free(history); // Don't forget to free allocated memory
    return 0;
}