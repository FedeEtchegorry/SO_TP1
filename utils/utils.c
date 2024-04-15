#include "./utils.h"

void perrorExit(const char* errMsg) {
  perror(errMsg);
  // exit(EXIT_FAILURE);
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
