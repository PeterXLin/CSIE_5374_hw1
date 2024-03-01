#include <ctype.h>  // for isdigit
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXCOM 100000  // max length of a commend
#define MAXARGS 4096   // _POSIX_ARG_MAX
#define MAXHISTORY 10  // max length of history

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

char* comHistory[MAXCOM];  // store history commend
int numberCommands = 0;

void printDir() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s > ", cwd);
}

void customCdHandler(char** parsed) {
    chdir(parsed[1]);
    return;
}

void customExitHandler(char** parsed) {
    // printf("Eed shell\n");
    exit(0);
}

int isNumber(char* input) {
    int length = strlen(input);
    for (int i = 0; i < length; i++) {
        if (!isdigit(input[i])) {
            return 0;
        }
    }
    return 1;
}

void printHistory(int number) {
    /* number must no bigger than 10*/
    if (number > MAXHISTORY) return printHistory(MAXHISTORY);

    if (numberCommands < MAXHISTORY) { /* numberCommands also no bigger than MAXHISTORY*/
        // int to = (numberCommands - 1) % 10;
        for (int i = MAX(0, numberCommands - number); i < numberCommands; i++) {
            printf("%5d  %s", i + 1, comHistory[i]);
        }
    } else {
        int printCount = 0;
        int modulus = (numberCommands - 1) % 10;
        int startIndex = modulus - number + 1;

        if (startIndex < 0) {
            for (int i = MAXHISTORY + startIndex; i < MAXHISTORY; i++) {
                printf("%5d  %s", numberCommands - modulus - number + i, comHistory[i]);
            }
        }

        for (int i = MAX(0, startIndex); i <= modulus; i++) {
            printf("%5d  %s", numberCommands - modulus + i, comHistory[i]);
        }
    }
}

void customHistoryHandler(char** parsed) {
    if (parsed[1] == NULL) {
        printHistory(MAXHISTORY);
    } else if (strcmp(parsed[1], "-c") == 0) {
        for (int i = 0; i < MAXHISTORY; i++) {
            free(comHistory[i]);
            comHistory[i] = NULL;
        }
        numberCommands = 0;
    } else if (isNumber(parsed[1])) {
        int number = atoi(parsed[1]);
        // printf("number: %d\n", number);
        printHistory(number);
    } else {
        fprintf(stderr, "error: %s\n", "history, invalid option");
    }

    return;
}

void addHistory(char* inputStr) {
    if (numberCommands != 0 && strcmp(comHistory[(numberCommands - 1) % 10], inputStr) == 0) return;

    numberCommands++;
    if (comHistory[(numberCommands - 1) % 10] != NULL) {
        free(comHistory[(numberCommands - 1) % 10]);
    }

    comHistory[(numberCommands - 1) % 10] = (char*)malloc(strlen(inputStr) + 1);
    strcpy(comHistory[(numberCommands - 1) % 10], inputStr);
    // printf("finish adding history\n");
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
    /* modify str and firstCommand, str point to next pipe's next, firstcommand point to the finded substring*/
    *firstCommand = strsep(str, "|");
    // printf("first command: %s\n", *firstCommand);

    if (*str == NULL) return 0; /* this command not contain pipe */

    return 1; /* this command has pipe*/
}

void execSingleCommand(char* inputStr) {
    char* parsed[MAXARGS];
    parseCommandBySpace(inputStr, parsed);

    if (customCommandHandler(parsed)) return;

    pid_t pid;

    if ((pid = fork()) < 0) {  // pid = fork need to be enclosed by parentheses
        fprintf(stderr, "error: %s\n", "fork error");
        return;
    } else if (pid > 0) { /* parent */
        wait(NULL);       // waiting for child process to terminate
        return;
    } else { /* child process */
        if (execvp(parsed[0], parsed) < 0) {
            fprintf(stderr, "error: %s\n", "execvp error");
            exit(EXIT_FAILURE);  // if execvp failed
        }
    }
}

int countPipe(char* inputStr) {
    char* nextPipe;
    int pipeCount = 0;
    while (1) {
        nextPipe = strchr(inputStr, '|');
        if (nextPipe == NULL) {
            break;
        } else {
            pipeCount++;
            inputStr = nextPipe + 1;
        }
    }
    return pipeCount;
}

void execPipedCommand2(char* inputStr) {
    // seem inputStr as a full str
    // printf("input command: %s\n", inputStr);
    int numberPipes = countPipe(inputStr);  // pipe amount we need
    int pipes[numberPipes][2];
    char* parsed[MAXARGS];
    // printf("Number of pipes: %d\n", numberPipes);

    for (int i = 0; i < numberPipes; i++) {
        if (pipe(pipes[i])) {
            fprintf(stderr, "error: %s\n", "pipe error");
            exit(EXIT_FAILURE);
        }
    }

    char* nextCommand;
    for (int commandCount = 0; commandCount <= numberPipes; commandCount++) {
        parseCommandByPipe(&inputStr, &nextCommand);
        parseCommandBySpace(nextCommand, parsed);
        int pid;

        if (strcmp(parsed[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
        }

        if ((pid = fork()) < 0) {
            fprintf(stderr, "error: %s\n", "fork error");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (commandCount != 0) {
                // printf("read from pipe");
                if (dup2(pipes[commandCount - 1][0], STDIN_FILENO) != 0) {
                    fprintf(stderr, "error: %s\n", "dup2 error");
                    exit(EXIT_FAILURE);
                }
            }

            if (commandCount != numberPipes) {
                // printf("write to pipe");
                if (dup2(pipes[commandCount][1], STDOUT_FILENO) != 1) {
                    fprintf(stderr, "error: %s\n", "dup2 error");
                    exit(EXIT_FAILURE);
                }
            }

            for (int i = 0; i < numberPipes; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            if (customCommandHandler(parsed)) {
                exit(0);
            }

            if (execvp(parsed[0], parsed) < 0) {
                fprintf(stderr, "error: %s\n", "execvp error");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < numberPipes; i++) {
        close(pipes[i][0]);  // parent process do not need pipe
        close(pipes[i][1]);
        wait(NULL);
    }

    wait(NULL);  // all numberPipes + 1 child process
    return;
}

int main() {
    char* inputStr;  // input commend
    size_t size = 0;

    char* parsedArgs[MAXARGS];  // inputStr is a constant, may be safer than just use a char*?
    // char* comHistory[MAXCOM];   // store history commend
    // int numberCommands = 0;

    int execFlag = 0;
    char* pipedCommand;

    while (1) {
        printf("$");
        printDir();

        if (getline(&inputStr, &size, stdin) < 2) {  // inputStr include \n
            continue;
        }

        addHistory(inputStr);
        inputStr[strlen(inputStr) - 1] = '\0';  // replace \n with \0

        int pipeAmount = countPipe(inputStr);
        // printf("pipeAmount: %d\n", pipeAmount);
        if (pipeAmount == 0) {
            execSingleCommand(inputStr);
        } else {
            execPipedCommand2(inputStr);
        }
    }

    free(inputStr);

    return 0;
}