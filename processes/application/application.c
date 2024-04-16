#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
// for ftruncate
#define __USE_XOPEN_EXTENDED
#include <globals.h>
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils.h>

// #define SHM_SIZE 8192
#define SHM_NAME "/results"
// User write, group read, other reads (is others necessary?)
#define SHM_PERMISSIONS S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH

#define BUFFER_SIZE 1000
#define CMD_BUFFER_SIZE 150
#define SLAVE_RAW_CMD "/slave"
#define ERROR -1
#define CHILD 0
#define FORK_QUANT 5

typedef int pipe_t[2];
enum { READ = 0, WRITE = 1 };

static int initializeChildren(int fileQuant, pipe_t sendTasks[], pipe_t getResults[], char slaveCmd[]);
static void stopChildren(int fileQuant, pipe_t sendTasks[], pipe_t getResults[]);
int writeToShm(char* shmBuf, char* result, sem_t* semaphore);

int main(int argc, char* argv[]) {

  // To save the path where the slave command is
  char* path = dirname(argv[0]);
  // To start with the paths
  argv++;

  int fileQuant = argc - 1;
  if (fileQuant < 1) {
    exitWithFailure("No files were passed\n");
  }

  int shmFd = safeShmOpen(SHM_NAME, O_CREAT | O_RDWR, SHM_PERMISSIONS);

  safeFtruncate(shmFd, SHM_SIZE);

  sem_t* semaphore = safeSemOpenCreate(SEM_NAME, SHM_PERMISSIONS, 0);

  // Set line buffering so the SHM_NAME gets sent as soon it's printed to stdout.
  setvbuf(stdout, NULL, _IOLBF, 0);
  puts(SHM_NAME);
  sleep(5);

  char* shmBuf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  if (shmBuf == MAP_FAILED) perrorExit("mmap() error");

  pipe_t sendTasks[FORK_QUANT];
  pipe_t getResults[FORK_QUANT];

  char slaveCmd[CMD_BUFFER_SIZE];
  strcpy(slaveCmd, path); strcat(slaveCmd, SLAVE_RAW_CMD); 
  
  int childAmount = initializeChildren(fileQuant, sendTasks, getResults, slaveCmd);

  FILE* file = fopen("results.txt", "w+");
  if (file == NULL) perrorExit("fopen() error");

  fd_set rfds;
  int maxFd = getResults[childAmount - 1][READ];
  char* shmBufCurrent = shmBuf;
  int sentTasksCount = 0;
  int resultsCount = 0;

  int i = 0;
  // We start sending tasks to the slave processes
  while (sentTasksCount < minInt(2 * FORK_QUANT, fileQuant)) {
    int length = strlen(argv[sentTasksCount]);
    // argv[sentTasksCount][length] = '\n';
    safeWrite(sendTasks[(i++) % FORK_QUANT][WRITE], argv[sentTasksCount], length + 1);
    sentTasksCount++;
  }

  while (resultsCount < fileQuant) {
    // Prepares the rfds since select is destructive
    FD_ZERO(&rfds);
    for (int j = 0; j < minInt(FORK_QUANT, fileQuant); j++) {
      FD_SET(getResults[j][READ], &rfds);
    }

    if (select(maxFd + 1, &rfds, NULL, NULL, NULL) == ERROR) perrorExit("select() error");

    for (int j = 0; j < minInt(FORK_QUANT, fileQuant); j++) {
      if (FD_ISSET(getResults[j][READ], &rfds)) {

        char result[BUFFER_SIZE] = {0};
        safeRead(getResults[j][READ], result, BUFFER_SIZE);
        if (fprintf(file, "%s", result) < 0) exitWithFailure("fprint() error");
        shmBufCurrent += writeToShm(shmBufCurrent, result, semaphore);
        resultsCount++;
        if (sentTasksCount < fileQuant) {
          int length = strlen(argv[sentTasksCount]);
          safeWrite(sendTasks[j][WRITE], argv[sentTasksCount], length + 1);
          sentTasksCount++;
        }
      }
    }
  }
  // Write empty string (basically \0) to shm so view can know where it ends.
  writeToShm(shmBufCurrent, "", semaphore);

  stopChildren(fileQuant, sendTasks, getResults);
  // getchar();

  // Si hacés ctrl+c antes de que se ejecute esto quedan la shm y el semaphore abiertos.
  // Se puede solucinoar eso? (con señales supongo pero suena paja)
  if (fclose(file) == ERROR) perrorExit("fclose() error");
  if (munmap(shmBuf, SHM_SIZE) == ERROR) perrorExit("munmap() error");
  if (sem_unlink(SEM_NAME) == ERROR) perrorExit("sem_unlink() error");
  if (shm_unlink(SHM_NAME) == ERROR) perrorExit("shm_unlink() error");

  exit(EXIT_SUCCESS);
}

int writeToShm(char* shmBuf, char* result, sem_t* semaphore) {
  int len = 0;
  while (result[len] != '\n') {
    shmBuf[len] = result[len];
    ++len;
  }
  shmBuf[len++] = '\n';
  sem_post(semaphore);
  return len;
}

/*
  The communication would be:
  from app sendTask[i][WRITE] -> to slave sendTask[i][READ]

  from slave getResults[i][WRITE] -> to app getResults[i][READ]

  with i belonging to {0, ..., min(FORK_QUANT-1, fileQuant)}
  */
static int initializeChildren(int fileQuant, pipe_t sendTasks[], pipe_t getResults[], char slaveCmd[]) {
  int pid;
  int i;
  for (i = 0; i < minInt(FORK_QUANT, fileQuant); i++) {
    safePipe(sendTasks[i]);
    safePipe(getResults[i]);
    pid = fork();
    if (pid == ERROR) perrorExit("Error while creating slave process\n");
    else if (pid == CHILD) {
      safeClose(READ);
      safeDup(sendTasks[i][READ]);
      safeClose(WRITE);
      safeDup(getResults[i][WRITE]);
      safeClose(sendTasks[i][READ]);
      safeClose(getResults[i][WRITE]);
      for (int j = 0; j <= i; j++) {
        safeClose(sendTasks[j][WRITE]);
        safeClose(getResults[j][READ]);
      }

      printf(slaveCmd);
      char* argv[] = {slaveCmd, NULL};
      char* envp[] = {NULL};
      execve(slaveCmd, argv, envp);
      perrorExit("execve() error");
    } else {
      safeClose(sendTasks[i][READ]);
      safeClose(getResults[i][WRITE]);
    }
  }
  return i;
}

static void stopChildren(int fileQuant, pipe_t sendTasks[], pipe_t getResults[]) {
  for (int i = 0; i < minInt(FORK_QUANT, fileQuant); i++) {
    // closing the w-end, leads to the r-end receiving EOF
    safeClose(sendTasks[i][WRITE]);
    safeClose(getResults[i][READ]);
  }
}

// https://www.tutorialspoint.com/unix_system_calls/_newselect.htm
// https://jameshfisher.com/2017/02/24/what-is-mode_t/
// https://man7.org/linux/man-pages/man2/mmap.2.html
// shared memory is stored in /dev/shm/results
