#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#define ERROR (-1)
// The buffer size is bigger than necessary to avoid overflowing the
// buffer (It will be considered that every fgets will not reach the
// last character of the BUFFER_SIZE)
#define BUFFER_SIZE 10000
#define SMALL_BUFFER 1000

void md5sum_caller(char* path, char* buffer);
void return_md5sum_result(char* md5sum_buffer);

int main(int argc, char* argv[]) {
  // Disable buffering on stdout so printf works on pipes.
  // setvbuf(stdout, NULL, _IONBF, 0);

  char filename_buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  char buf[SMALL_BUFFER];
  int j = 0;
  while ((bytes_read = safeRead(STDIN_FILENO, filename_buffer, SMALL_BUFFER)) != 0) {
    for (int i = 0; i < bytes_read; i++) {
      if (filename_buffer[i] == '\n' || filename_buffer[i] == 0) {
        char md5sum_buffer[SMALL_BUFFER];
        buf[j] = '\0';
        j = 0;
        md5sum_caller(buf, md5sum_buffer);
        return_md5sum_result(md5sum_buffer);
      } else {
        buf[j++] = filename_buffer[i];
      }
    }
  }
  return 0;
}

void md5sum_caller(char* path, char* buffer) {
  char md5sum_command[SMALL_BUFFER] = "md5sum ";
  // Add the file path to the md5sum_command
  strcpy(md5sum_command + strlen(md5sum_command), path);
  FILE* fp = popen(md5sum_command, "r");
  if (fp == NULL) perrorExit("peopen() error");
  if (fgets(buffer, SMALL_BUFFER, fp) == NULL) perrorExit("fgets() error");
  buffer[strlen(buffer) - 1] = '\0';
  if (pclose(fp) == -1) perrorExit("pclose() error");
}

void return_md5sum_result(char* md5sum_buffer) {
  char ret_md5sum[SMALL_BUFFER];
  snprintf(ret_md5sum, SMALL_BUFFER, "%s - PROCESS PID %d\n", md5sum_buffer, getpid());
  write(STDOUT_FILENO, ret_md5sum, SMALL_BUFFER);
}
