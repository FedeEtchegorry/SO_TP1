#include "./utils.h"

// General purpose functions

int minInt(int x, int y) { return x < y ? x : y; }

// Wrapper functions used for managing errors

void perrorExit(const char* errMsg) {
  perror(errMsg);
  exit(errno);
}

void exitWithFailure(const char* errMsg) {
  fprintf(stderr, "%s", errMsg);
  exit(EXIT_FAILURE);
}

size_t safeRead(int fd, void* buf, size_t nbytes) {
  size_t r = read(fd, buf, nbytes);
  if (r < 0) perrorExit("read error");
  return r;
}

size_t safeWrite(int fd, void* buf, size_t nbytes) {
  size_t w = write(fd, buf, nbytes);
  if (w < 0) perrorExit("read error");
  return w;
}

void safePipe(int pipedes[2]) {
  if (pipe(pipedes) < 0) perrorExit("pipe() error");
}

void safeClose(int fd) {
  if (close(fd) < 0) perrorExit("close() error");
}
void safeDup(int fd){
  if (dup(fd) < 0) perrorExit("close() error");
}