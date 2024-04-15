#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
// for ftruncate
#define __USE_XOPEN_EXTENDED
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define SHM_SIZE 8192
#define SHM_NAME "/results"
#define SLAVE_CMD "./slave"
#define RW_MODE 0666
#define ERROR -1
#define CHILD 0
#define FORK_QUANT 5
typedef int pipe_t[2];
enum { READ = 0, WRITE = 1 };

static int
initializeChilds(int fileQuant, pipe_t sendTasks[], pipe_t getResults[]);
static int minInt(int x, int y);
// can be a function for all files
static void exitWithFailure(const char* errMsg);

int main(int argc, char* argv[]) {
  // To start with the paths
  argv++;
  int fileQuant = argc - 1;
  if (fileQuant < 1) exitWithFailure("No files were passed\n");

  int shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, RW_MODE);
  if (shmFd == ERROR) exitWithFailure("Error while creating shm\n");

  if (ftruncate(shmFd, SHM_SIZE) == ERROR)
    exitWithFailure("Error while truncating shm\n");

  puts(SHM_NAME);
  sleep(5);

  char* shmBuf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  if (shmBuf == MAP_FAILED) exitWithFailure("Error while mapping shm\n");

  pipe_t sendTasks[FORK_QUANT];
  pipe_t getResults[FORK_QUANT];

  int childAmount = initializeChilds(fileQuant, sendTasks, getResults);

  fd_set rfds;
  int maxFd = getResults[childAmount - 1][READ];
  char* iniAddress = shmBuf;
  int sentTasksCount = 0;
  int resultsCount = 0;

  // We start sending tasks to the slave processes
  while (sentTasksCount < minInt(FORK_QUANT, fileQuant)) {
    int length = strlen(argv[sentTasksCount]);
    // argv[sentTasksCount][length] = '\n';
    write(sendTasks[sentTasksCount][WRITE], argv[sentTasksCount], length + 1);
    sentTasksCount++;
  }

  while (resultsCount < fileQuant) {
    // Prepares the rfds since select is destructive
    FD_ZERO(&rfds);
    for (int j = 0; j < minInt(FORK_QUANT, fileQuant); j++) {
      FD_SET(getResults[j][READ], &rfds);
    }

    if (select(maxFd + 1, &rfds, NULL, NULL, NULL) == ERROR)
      exitWithFailure("Error while running select function\n");

    for (int j = 0; j < minInt(FORK_QUANT, fileQuant); j++) {
      if (FD_ISSET(getResults[j][READ], &rfds)) {
        shmBuf += read(getResults[j][READ], shmBuf, SHM_SIZE - (shmBuf - iniAddress));
        resultsCount++;
        if (sentTasksCount < fileQuant) {
          int length = strlen(argv[sentTasksCount]);
          write(sendTasks[j][WRITE], argv[sentTasksCount], length + 1);
          sentTasksCount++;
        }
      }
    }
  }

  for (int i = 0; i < minInt(FORK_QUANT, fileQuant); i++) {
    // closing the w-end, leads to the r-end receiving EOF
    close(sendTasks[i][WRITE]);
    close(getResults[i][READ]);
  }

  if (munmap(NULL, SHM_SIZE) == ERROR) {
    perror("munmap() error");
    exitWithFailure("Error while unmapping shm\n");
  }

  if (shm_unlink(SHM_NAME) == ERROR) exitWithFailure("Error while closing shm\n");

  exit(EXIT_SUCCESS);
}

/*
  The communication would be:
  from app sendTask[i][WRITE] -> to slave sendTask[i][READ]

  from slave getResults[i][WRITE] -> to app getResults[i][READ]

  with i belonging to {0, ..., min(FORK_QUANT-1, fileQuant)}
  */
static int
initializeChilds(int fileQuant, pipe_t sendTasks[], pipe_t getResults[]) {
  int pid;
  int i;
  for (i = 0; i < minInt(FORK_QUANT, fileQuant); i++) {
    if (pipe(sendTasks[i]) == ERROR)
      exitWithFailure("Error while creating pipe\n");
    if (pipe(getResults[i]) == ERROR)
      exitWithFailure("Error while creating pipe\n");
    pid = fork();
    if (pid == ERROR) exitWithFailure("Error while creating slave process\n");
    else if (pid == CHILD) {
      close(READ);
      dup(sendTasks[i][READ]);
      close(WRITE);
      dup(getResults[i][WRITE]);
      close(sendTasks[i][READ]);
      close(getResults[i][WRITE]);
      for (int j = 0; j <= i; j++) {
        close(sendTasks[j][WRITE]);
        close(getResults[j][READ]);
      }

      char* argv[] = {SLAVE_CMD, NULL};
      char* envp[] = {NULL};
      execve(SLAVE_CMD, argv, envp);
      exitWithFailure("Error while calling slave process\n");
    } else {
      close(sendTasks[i][READ]);
      close(getResults[i][WRITE]);
    }
  }
  return i;
}

static int minInt(int x, int y) { return x < y ? x : y; }

static void exitWithFailure(const char* errMsg) {
  fprintf(stderr, "%s", errMsg);
  exit(EXIT_FAILURE);
}

// https://www.tutorialspoint.com/unix_system_calls/_newselect.htm
// https://jameshfisher.com/2017/02/24/what-is-mode_t/
// https://man7.org/linux/man-pages/man2/mmap.2.html
// shared memory is stored in /dev/shm/results
