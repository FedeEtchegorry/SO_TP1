#include <globals.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <utils.h>

#define PID_BUFFER 10
#define ERROR (-1)
#define SHM_NAME_BUF_SIZE 100

int readFromShm(char* buf, sem_t* sem);

int main(int argc, char* argv[]) {
  // I'll do it with getchar to avoid replacing the '\n'
  char shmName[SHM_NAME_BUF_SIZE];
  int shmNameLen = 0;
  int c;
  while ((c = getchar()) != '\n' && c != EOF) shmName[shmNameLen++] = c;
  shmName[shmNameLen] = 0;

  int shmFd = safeShmOpen(shmName, O_RDONLY, 0);
  char* const shmBuf = safeMmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shmFd, 0);
  char* shmBufCurrent = shmBuf;

  sem_t* enabledForRead = safeSemOpenRead(SEM_NAME);

  int resultLength = 0;
  do {
    resultLength = readFromShm(shmBufCurrent, enabledForRead);
    shmBufCurrent += resultLength;
  } while (resultLength > 0);

  safeSemClose(enabledForRead);
  if (close(shmFd) < 0) perrorExit("close() error");
  safeMunmap(shmBuf, SHM_SIZE);

  return 0;
}

int readFromShm(char* buf, sem_t* sem) {
  sem_wait(sem);
  int len = 0;
  while (buf[len] != '\n' && buf[len] != 0) {
    putchar(buf[len++]);
  }
  if (buf[len] == 0) return 0;
  putchar('\n');
  return ++len;
}