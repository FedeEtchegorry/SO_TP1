#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define ERROR (-1)
#define BUFFER_SIZE 10000                                             //The buffer size is bigger than necessary to avoid overflowing the buffer (It will be considered that every fgets will not reach the last character of the BUFFER_SIZE)
#define SMALL_BUFFER 100

FILE *popen(const char *command, const char *type);                 //Must be included to avoid warnings
int pclose(FILE *stream);

static void exitWithFailure(const char *errMsg);

void md5sum_caller(char *path, char *buffer);
void return_md5sum_result(char* md5sum_buffer);


int main(int argc, char *argv[]) {
    char filename_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = read(STDIN_FILENO, filename_buffer, SMALL_BUFFER)) != 0) {
        if (bytes_read==ERROR){
            exitWithFailure("READ failed");
        }
        int position = 0;
        for (int i = 0; i < bytes_read; i++) {
            if (filename_buffer[i] == '\0' && filename_buffer[i-1]!='\0') {
                char md5sum_buffer[SMALL_BUFFER];
                md5sum_caller(&filename_buffer[position], md5sum_buffer);
                position = i+1;
                md5sum_buffer[strlen(md5sum_buffer)-1]='\0';
                return_md5sum_result(md5sum_buffer);
            }
        }
    }
    return 0;
}

void md5sum_caller(char *path, char *buffer) {
    char md5sum_command[BUFFER_SIZE] = "md5sum ";                                                 //The md5sum call to be made
    strcpy(md5sum_command + strlen(md5sum_command),
           path);                  //Add the file path to the md5sum_command
    FILE *fp = popen(md5sum_command,
                     "r");                              //Popen makes the fork call and creates the pipe, r means reading mode
    if (fp == NULL)                                                                  //check Popen failure
        exitWithFailure("POPEN couldn´t be created successfully");
    if (fgets(buffer, BUFFER_SIZE, fp) ==NULL)                            //Fgets fills the buffer with the md5sum result
        exitWithFailure("FGETS was unsuccessful to read the data into the buffer");         //Error handling
    if (pclose(fp) == -1)
        exitWithFailure("PCLOSE couldn´t close successfully");
}



static void exitWithFailure(const char *errMsg) {
    perror(errMsg);
    exit(EXIT_FAILURE);
}
void return_md5sum_result(char* md5sum_buffer) {
    size_t length = snprintf(NULL, 0, "%s - PROCESS PID %d\n", md5sum_buffer, getpid())+1;
    char ret_md5sum[length];
    snprintf(ret_md5sum, length, "%s - PROCESS PID %d\n", md5sum_buffer, getpid());
    write(STDOUT_FILENO, ret_md5sum, length);
}
