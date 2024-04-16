#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#define ERROR (-1)
// The buffer size is bigger than necessary to avoid overflowing the
// buffer (It will be considered that every fgets will not reach the
// last character of the BUFFER_SIZE)
#define BUFFER_SIZE 50000
#define SMALL_BUFFER 1000

void md5sumCaller(char* path, char* buffer);
void returnMd5sumResult(char* md5sum_buffer);

int main(int argc, char* argv[]) {
  char filenameBuffer[BUFFER_SIZE];
  ssize_t bytesRead;
  char buf[SMALL_BUFFER];
  int j = 0;
  while ((bytesRead = safeRead(STDIN_FILENO, filenameBuffer, SMALL_BUFFER)) != 0) {
    for (int i = 0; i < bytesRead; i++) {
      if (filenameBuffer[i] == '\n' || filenameBuffer[i] == 0) {
        char md5sumBuffer[SMALL_BUFFER];
        buf[j] = '\0';
        j = 0;
        md5sumCaller(buf, md5sumBuffer);
        returnMd5sumResult(md5sumBuffer);
      } else {
        buf[j++] = filenameBuffer[i];
      }
    }
  }
  return 0;
}

void md5sumCaller(char* path, char* buffer) {
  char md5sumCommand[SMALL_BUFFER] = "md5sum ";
  // Add the file path to the md5sum_command
  strcpy(md5sumCommand + strlen(md5sumCommand), path);
  FILE* fp = popen(md5sumCommand, "r");
  if (fp == NULL) perrorExit("peopen() error");
  if (fgets(buffer, SMALL_BUFFER, fp) == NULL) perrorExit("fgets() error");
  buffer[strlen(buffer) - 1] = '\0';
  if (pclose(fp) == -1) perrorExit("pclose() error");
}

void returnMd5sumResult(char* md5sumBuffer) {
  char retMd5sum[SMALL_BUFFER];
  snprintf(retMd5sum, SMALL_BUFFER, "%s - PROCESS PID %d\n", md5sumBuffer, getpid());
  write(STDOUT_FILENO, retMd5sum, SMALL_BUFFER);
}
