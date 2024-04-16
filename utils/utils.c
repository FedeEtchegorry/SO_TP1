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

ssize_t safeRead(int fd, void* buf, size_t nbytes) {
  ssize_t r = read(fd, buf, nbytes);
  if (r < 0) perrorExit("read error");
  return r;
}

ssize_t safeWrite(int fd, void* buf, size_t nbytes) {
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

void safeDup(int fd) {
  if (dup(fd) < 0) perrorExit("close() error");
}

int safeShmOpen(const char* name, int oflag, mode_t mode) {
  int shmFd = shm_open(name, oflag, mode);
  if (shmFd < 0) perrorExit("shm_open() error");
  return shmFd;
}

void safeFtruncate(int fd, __off_t length) {
  if (ftruncate(fd, length) < 0) exitWithFailure("ftruncate() error");
}

char* safeMmap(void* addr, size_t len, int prot, int flags, int fd, __off_t offset) {
  char* shmBuf = mmap(addr, len, prot, flags, fd, offset);
  if (shmBuf == MAP_FAILED) perrorExit("mmap() error");
  return shmBuf;
}

void safeMunmap(void* addr, size_t len) {
  if (munmap(addr, len) < 0) perrorExit("munmap() error");
}

sem_t* safeSemOpenCreate(const char* name, mode_t permissions, int initialValue) {
  sem_t* semaphore = sem_open(name, O_CREAT, permissions, initialValue);
  if (semaphore == SEM_FAILED) perrorExit("sem_open() error on create");
  return semaphore;
}

sem_t* safeSemOpenRead(const char* name) {
  sem_t* semaphore = sem_open(name, 0);
  if (semaphore == SEM_FAILED) perrorExit("sem_open() error on read");
  return semaphore;
}

void safeSemClose(sem_t* sem) {
  if (sem_close(sem) < 0) perrorExit("sem_close() error");
}
