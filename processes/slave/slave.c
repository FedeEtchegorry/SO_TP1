#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

//#define PID_ERROR (-1)
//#define PARENT_PID 0                                             //The pid 0 refers to the parent process but the parent pid is stored by the child
//#define EXECVE_ERROR (-1)

#define ERROR (-1)

#define BUFFER_SIZE 256                                             //The buffer size is bigger than necessary to avoid overflowing the buffer (It will be considered that every fgets will not reach the last character of the BUFFER_SIZE)

FILE *popen(const char *command, const char *type);                 //Must be included to avoid warnings
int pclose(FILE *stream);

static void exitWithFailure(const char *errMsg);

void md5sum_caller(char *path, char *buffer);

int main(int argc, char *argv[]) {
    int eof_flag = 0;
    fd_set fdSet;
    while (!eof_flag) {                                                                              //The slave will process a number of files sent by the master one at the time, so with select I avoid busy waiting
        FD_ZERO(&fdSet);
        FD_SET(STDIN_FILENO, &fdSet);
        int val = select(1, &fdSet, NULL, NULL, NULL);
        if (val == ERROR)
            exitWithFailure("SELECT didn´t work as expected");
        if (val) {
            char path[BUFFER_SIZE];
            if (fgets(path, BUFFER_SIZE, stdin) == NULL) {
                if (feof(stdin))
                    eof_flag = 1;
                else if (ferror(stdin))
                    exitWithFailure("FGETS was unsuccessful to read the data into the buffer");
            } else {
                path[strlen(path)-1]='\0';
                char buffer[BUFFER_SIZE] = {'\0'};
                md5sum_caller(path, buffer);
                buffer[strlen(buffer) -1] = '\0';                                      //I remove the newline created by fgets
                printf("%s - PROCESS PID %d\n", buffer, getpid());
            }
        }
    }
}

void md5sum_caller(char *path, char *buffer) {
    char md5sum_command[BUFFER_SIZE] = "md5sum ";                                                 //The md5sum call to be made
    strcpy(md5sum_command + strlen(md5sum_command),
           path);                  //Add the file path to the md5sum_command
    FILE *fp = popen(md5sum_command,
                     "r");                              //Popen makes the fork call and creates the pipe, r means reading mode
    if (fp == NULL)                                                                  //check Popen failure
        exitWithFailure("POPEN couldn´t be created successfully");
    if (fgets(buffer, BUFFER_SIZE, fp) ==
        NULL)                            //Fgets fills the buffer with the md5sum result
        exitWithFailure("FGETS was unsuccessful to read the data into the buffer");         //Error handling
    if (pclose(fp) == -1)
        exitWithFailure("PCLOSE couldn´t close successfully");
}



static void exitWithFailure(const char *errMsg) {
    perror(errMsg);
    exit(EXIT_FAILURE);
}
