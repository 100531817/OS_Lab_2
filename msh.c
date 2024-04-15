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
#include <ctype.h>

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

/* Function Declarations for helper functions */
void execute_command_sequence(char ***argvv, char filev[3][64], int num_commands, int in_background);

void getCompleteCommand(char*** argvv, int num_command);
/* Function Declarations for helper functions */

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
 * Checks if a given string represents a valid integer.
 * @param str
 * @return Returns 1 if the string is a valid integer, 0 otherwise.
 */
int is_valid_integer(const char *str) {
    // Handle negative numbers and skip sign
    if (*str == '-' || *str == '+') str++;
    // Check for empty string after the sign
    if (!*str) return 0;
    // Check each character to ensure it's a digit
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1; // Every character was a digit
}

/* mycalc */
/**
 * Function to perform the arithmetic operation and print the result
 * @param operand1_str
 * @param operation
 * @param operand2_str
 * @return
 */
void execute_mycalc(const char* operand1_str, const char* operation, const char* operand2_str) {
    // Check if operands are valid integers
    if (!is_valid_integer(operand1_str) || !is_valid_integer(operand2_str)) {
        printf("[ERROR] Both operands must be valid integers.\n");
        return;
    }

    int operand1 = atoi(operand1_str);
    int operand2 = atoi(operand2_str);
    int result;
    static int acc = 0; // Static variable to keep track of the accumulated sum

    // Determine the operation
    if (strcmp(operation, "add") == 0) {
        result = operand1 + operand2;
        acc += result; // Accumulate the sum
        fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", operand1, operand2, result, acc);
    } else if (strcmp(operation, "mul") == 0) {
        result = operand1 * operand2;
        fprintf(stderr, "[OK] %d * %d = %d\n", operand1, operand2, result);
    } else if (strcmp(operation, "div") == 0) {
        if (operand2 == 0) {
            printf("[ERROR] Division by zero is not allowed\n");
            return;
        }
        int quotient = operand1 / operand2;
        int remainder = operand1 % operand2;
        fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2, quotient, remainder);
    } else {
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
    }
}

/**
 * Check if the command is mycalc and execute it
 * @param argvv
 * @param filev
 * @param in_background
 * @return
 */
int check_and_execute_mycalc(char*** argvv, char filev[3][64], int in_background) {
    // Check if the first argument is "mycalc"
    if (strcmp(argvv[0][0], "mycalc") == 0) {
        // Check that it is not a background command and there is no redirection
        if (strcmp(filev[0], "0") != 0 || strcmp(filev[1], "0") != 0 ||
            strcmp(filev[2], "0") != 0 || in_background) {
            printf("[ERROR] mycalc does not support background execution or redirection.\n");
            return 1; // Still return 1 to prevent further processing
        }
        // Check if there are exactly 4 arguments: "mycalc", "operand1", "operation", "operand2"
        if (argvv[0][1] != NULL && argvv[0][2] != NULL && argvv[0][3] != NULL && argvv[0][4] == NULL) {
            execute_mycalc(argvv[0][1], argvv[0][2], argvv[0][3]);
        } else {
            printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
        }
        return 1;
    }
    return 0;
}
/* mycalc */

/* myhistory */
/**
 * Executes the desired command from history.
 * @param index
 * @return
 */
int execute_history_command(int index) {
    if (index < 0 || index >= n_elem || index >= history_size) {
        printf("ERROR: Command not found\n");
        return 1;
    }

    int pos = (head + index) % history_size;
    struct command *cmd = &history[pos];

    fprintf(stderr, "Running command %d\n", index);
    execute_command_sequence(cmd->argvv, cmd->filev, cmd->num_commands, cmd->in_background);
    return 0;
}

/**
 * Checks which version of myhistory was called.
 * Displays user command history if no input specified.
 * @param argv
 * @return
 */
void myhistory(char **argv) {
    if (argv[1] == NULL) {
        // Display the last 20 commands
        for (int i = 0; i < n_elem; i++) {
            int pos = (head + i) % history_size;
            fprintf(stderr, "%d ", i);  // Print the command index
            // Loop through each command sequence
            for (int j = 0; j < history[pos].num_commands; j++) {
                // Loop through each argument in the command
                if (j > 0) fprintf(stderr, "| ");  // Add separator for command sequences
                for (int k = 0; k < history[pos].args[j]; k++) {
                    fprintf(stderr, "%s ", history[pos].argvv[j][k]);
                }
            }
            // Display redirections and background flag using `filev` array similar to command execution
            if (strcmp(history[pos].filev[0], "0") != 0) {
                fprintf(stderr, "< %s ", history[pos].filev[0]);  // Input redirection
            }
            if (strcmp(history[pos].filev[1], "0") != 0) {
                fprintf(stderr, "> %s ", history[pos].filev[1]);  // Output redirection
            }
            if (strcmp(history[pos].filev[2], "0") != 0) {
                fprintf(stderr, "!> %s ", history[pos].filev[2]);  // Error redirection
            }
            if (history[pos].in_background) {
                fprintf(stderr, "&");  // Check if the command was run in background
            }
            fprintf(stderr, "\n");  // New line after each full command sequence
        }
    } else {
        // Execute a specific command from history
        int index = atoi(argv[1]);
        execute_history_command(index);
    }
}
/* myhistory */

/**
 * Provides basic command functionality, command sequence functionality,
 * background support, and redirection support.
 * @param argvv
 * @param filev
 * @param num_commands
 * @param in_background
 * @return
 */
void execute_command_sequence(char ***argvv, char filev[3][64], int num_commands, int in_background) {
    pid_t manager_pid = 0;
    if (in_background) {
        manager_pid = fork(); // Fork a manager process for background tasks
    }

    if (manager_pid == 0 || !in_background) { // Proceed if it's a foreground task or inside the manager process
        int pipe_fds[2 * (num_commands - 1)]; // Array to hold the file descriptors for all pipes

        // Create all needed pipes
        for (int i = 0; i < num_commands - 1; i++) {
            if (pipe(pipe_fds + i * 2) < 0) {
                perror("Couldn't Pipe");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < num_commands; i++) {
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
                if (i == num_commands - 1 && strcmp(filev[1], "0") != 0) {
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
                if (i != num_commands - 1) {
                    if (dup2(pipe_fds[i * 2 + 1], STDOUT_FILENO) < 0) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }

                // Close all pipe file descriptors
                for (int j = 0; j < 2 * (num_commands - 1); j++) {
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
                if (i < num_commands - 1) {
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
        for (int i = 0; i < 2 * (num_commands - 1); i++) {
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

/**
 * New SIGCHLD handler to avoid zombie processes
 * @param sig
 * @return
 */
void sigchld_handler(int sig) {
    // Use waitpid in a loop to reap all exited children
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

/**
 * Main shell Loop
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
    struct command cmd;

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
            // First, handle internal commands like 'mycalc' and 'myhistory'
            if (strcmp(argvv[0][0], "myhistory") == 0) {
                myhistory(argvv[0]);
            } else {
                int internal_cmd_handled = check_and_execute_mycalc(argvv, filev, in_background);
                if (!internal_cmd_handled) {
                    if (command_counter > MAX_COMMANDS) {
                        printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
                    } else {
                        // Check if we need to overwrite an old command in the history
                        if (n_elem == history_size) {  // History buffer is full
                            free_command(&history[head]);  // Free the resources of the command at 'head'
                        }

                        // Store the command into the current position in the history array
                        store_command(argvv, filev, in_background, &history[tail]);

                        // Update history tracking indices
                        tail = (tail + 1) % history_size;
                        if (n_elem < history_size) {
                            n_elem++;
                        } else {
                            head = (head + 1) % history_size;  // Move head if buffer is full
                        }
                        execute_command_sequence(argvv, filev, command_counter, in_background);  // Execute the command sequence
                    }
                }
            }
        }
    }
    // Final cleanup: Free all commands in history
    for (int i = 0; i < history_size; i++) {
        if (history[i].argvv != NULL) {
            free_command(&history[i]);
        }
    }
    free(history); // Don't forget to free allocated memory
    return 0;
}