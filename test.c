#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXCOM 100000  // max length of a commend
#define MAXARGS 4096   // _POSIX_ARG_MAX
#define MAXHISTORY 10  // max length of history

void printDir() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s > ", cwd);
}

void customCdHandler(char** parsed) {
    chdir(parsed[1]);
    return;
}

void customExitHandler() {
    printf("Eed shell\n");
    exit(0);
}

void customHistoryHandler() {
    // TODO handle history command
    printf("command handler: history\n");
    return;
}

int customCommandHandler(char** parsed) {
    // check command type, if input command is custom command, handle it and return 0
    // else return status number 1

    char* customCommandList[] = {"cd", "exit", "history"};  // if need to add more custom command, can add the command here
    int customCommandAmount = sizeof(customCommandList) / sizeof(char*);
    int commandIndex = 0;

    for (int i = 0; i < customCommandAmount; i++) {
        if (strcmp(parsed[0], customCommandList[i]) == 0) {
            commandIndex = i + 1;
            break;
        }
    }

    switch (commandIndex) {  // call each command's handler
        case 1:
            customCdHandler(parsed);
            break;
        case 2:
            customExitHandler(parsed);
            break;
        case 3:
            customHistoryHandler(parsed);
            break;
        default:
            return 0;  // not custom command
            break;
    }

    return 1;
    // for (int i = 0; i < )
}

// split command by space and store splited command
void parseCommandBySpace(char* str, char** parsed) {
    for (int i = 0; i < MAXARGS; i++) {
        parsed[i] = strsep(&str, " ");  // strsep not only modify string but also modify the address of str

        if (parsed[i] == NULL) break;  // finish parsing commandbreak;

        if (strlen(parsed[i]) == 0) i--;  // consecutive space
    }
}

int parseCommandByPipe(char** str, char** firstCommand) {
    *firstCommand = strsep(str, "|");
    // printf("first command: %s\n", *firstCommand);

    if (*str == NULL) return 0; /* this command not contain pipe */

    return 1; /* this command has pipe*/
}

int processCommand(char** inputStr, char** parsed) {  // char** because parsed is a char* array
    int pipeStatus = 0;                               // if pipeStatus == 1, it means command have pipe
    char* firstCommand = NULL;

    pipeStatus = parseCommandByPipe(inputStr, &firstCommand);

    printf("Pipe status: %d\n", pipeStatus);
    printf("first command: %s\n", firstCommand);

    // if (pipeStatus) { /* if pipe exist, only parse the first command */
    //     parseCommandBySpace(firstCommand, parsed);
    // } else {
    //     parseCommandBySpace(inputStr, parsed);
    // }
    parseCommandBySpace(firstCommand, parsed);

    // for (int i = 0; i < MAXARGS; i++) {
    //     if (parsed[i] == NULL) break;
    //     printf("%d-th: %s\n", i, parsed[i]);
    // }

    if (customCommandHandler(parsed)) {
        return 0;
    } else {
        return 1 + pipeStatus;
    }
}

void execSingleCommand(char** parsed) {
    pid_t pid;

    if ((pid = fork()) < 0) {  // pid = fork need to be enclosed by parentheses
        printf("fork error");
        return;
    } else if (pid > 0) { /* parent */
        wait(NULL);       // waiting for child process to terminate
        return;
    } else { /* child process */
        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command\n");
            exit(0);  // if execvp failed
        }
    }
}

// call this function only if | is in command
void execPipedCommand(char* inputStr, char** parsed) {
    // char* parsedArgs[MAXARGS];
    char* firstPart = NULL;

    int fd[2];
    pid_t pid = 0;

    if (pipe(fd) < 0) {
        printf("Could not create pipe");
        return;
    }
    /* execute first command, read from stdin, write to pip write end */
    if ((pid = fork()) < 0) {
        printf("Could not fork");
    }

    if (pid == 0) {
        printf("execute first command\n");
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute first command");
            exit(0);
        }
    }

    wait(NULL);  // wait child process

    /* execute middle command, read from read end of pipe, write to write end of pipe*/
    while (inputStr != NULL) {  // run all the command is pipe to stdout is available
        // printf("Parsing Command: %s\n", inputStr);
        firstPart = strsep(&inputStr, "|");

        if (firstPart == NULL || inputStr == NULL) break;

        if (strlen(firstPart) == 0) continue;

        printf("Command: %s\n", firstPart);
    }

    /* execute last command, read from pipe read end, write to STDOUT*/
    printf("last command: %s\n", firstPart);
    parseCommandBySpace(firstPart, parsed);

    // for (int i = 0; i < MAXARGS; i++) {
    //     if (parsed[i] == NULL) break;
    //     printf("%d-th: %s\n", i, parsed[i]);
    // }

    if ((pid = fork()) < 0) {
        printf("Could not create pipe");
        exit(0);
    }

    if (pid == 0) {
        printf("execute last command: %s", firstPart);
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute last command");
            exit(0);
        }
    }

    close(fd[0]);  // need to close the pipe, or the child won't receive EOF?
    close(fd[1]);
    wait(NULL);

    // write input

    return;
}

int main() {
    char* inputStr;  // input commend
    size_t size = 0;

    char* parsedArgs[MAXARGS];            // inputStr is a constant, may be safer than just use a char*?
    char comHistory[MAXHISTORY][MAXCOM];  // store history commend

    int execFlag = 0;
    char* pipedCommand;

    while (1) {
        printf("$");
        printDir();

        if (getline(&inputStr, &size, stdin) < 2) {  // inputStr include \n
            continue;
        }

        inputStr[strlen(inputStr) - 1] = '\0';
        // TODO copy command and store to history (because inputStr will be modified later)

        execFlag = processCommand(&inputStr, parsedArgs);

        printf("Command: %s\n", inputStr);
        if (execFlag == 1) {  // not custom simple command
            execSingleCommand(parsedArgs);
        } else if (execFlag == 2) {  // command with pipe
            execPipedCommand(inputStr, parsedArgs);
        }
    }

    free(inputStr);

    return 0;
}