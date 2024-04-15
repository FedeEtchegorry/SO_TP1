#include <fcntl.h>
#include <libgen.h>
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
#include <utils.h>

#define SHM_SIZE 8192
#define BUFFER_SIZE 100
#define SHM_NAME "/results"
#define SLAVE_CMD "./slave"
#define RW_MODE 0666
#define ERROR -1
#define CHILD 0
#define FORK_QUANT 5

typedef int pipe_t[2];
enum { READ = 0, WRITE = 1 };

static int initializeChildren(int fileQuant, pipe_t sendTasks[], pipe_t getResults[]);
static int minInt(int x, int y);

int main(int argc, char* argv[]) {
  char* path = dirname(argv[0]);
  char* binName = basename(argv[0]);
  pid_t pid = getpid();
  // To start with the paths
  argv++;
  int fileQuant = argc - 1;
  if (fileQuant < 1) {
    exitWithFailure("No files were passed\n");
  }

  int shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, RW_MODE);
  if (shmFd == ERROR) perrorExit("Error while creating shm");

  if (ftruncate(shmFd, SHM_SIZE) == ERROR) perrorExit("ftruncate() error");

  sem_t* smthAvailableToRead = sem_open("smthAvailableToRead", O_CREAT, RW_MODE, 0);
  if (smthAvailableToRead == SEM_FAILED) perrorExit("sem_open() error");

  puts(SHM_NAME);
  sleep(5);

  char* shmBuf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  if (shmBuf == MAP_FAILED) perrorExit("mmap() error");

  pipe_t sendTasks[FORK_QUANT];
  pipe_t getResults[FORK_QUANT];

  int childAmount = initializeChildren(fileQuant, sendTasks, getResults);

  FILE* file = fopen("results.txt", "w+");
  if (file == NULL) perrorExit("fopen() error");

  fd_set rfds;
  int maxFd = getResults[childAmount - 1][READ];
  char* iniAddress = shmBuf;
  int sentTasksCount = 0;
  int resultsCount = 0;

  // We start sending tasks to the slave processes
  while (sentTasksCount < minInt(FORK_QUANT, fileQuant)) {
    int length = strlen(argv[sentTasksCount]);
    // argv[sentTasksCount][length] = '\n';
    safeWrite(sendTasks[sentTasksCount][WRITE], argv[sentTasksCount], length + 1);
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

        char array[BUFFER_SIZE];
        safeRead(getResults[j][READ], array, sizeof(array));
        // Esto ta mal, del man page: If an output error is encountered, a negative value is returned
        // Osea hay que ver que sea menor a 0, no que sea -1.
        // if (fprintf(file, "%s", array) == ERROR) perrorExit("fprint error");
        if (fprintf(file, "%s", array) < 0) exitWithFailure("fprint() error");
        strcpy(shmBuf, array);
        shmBuf += strlen(array) + 1;

        sem_post(smthAvailableToRead);
        resultsCount++;
        if (sentTasksCount < fileQuant) {
          int length = strlen(argv[sentTasksCount]);
          safeWrite(sendTasks[j][WRITE], argv[sentTasksCount], length + 1);
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

  if (fclose(file) == ERROR) perrorExit("fclose() error");

  if (sem_close(smthAvailableToRead) == ERROR) perrorExit("sem_close() error");

  if (munmap(NULL, SHM_SIZE) == ERROR) perrorExit("munmap() error");

  if (shm_unlink(SHM_NAME) == ERROR) perrorExit("shm_unlink() error");

  exit(EXIT_SUCCESS);
}

/*
  The communication would be:
  from app sendTask[i][WRITE] -> to slave sendTask[i][READ]

  from slave getResults[i][WRITE] -> to app getResults[i][READ]

  with i belonging to {0, ..., min(FORK_QUANT-1, fileQuant)}
  */
static int initializeChildren(int fileQuant, pipe_t sendTasks[], pipe_t getResults[]) {
  int pid;
  int i;
  for (i = 0; i < minInt(FORK_QUANT, fileQuant); i++) {
    safePipe(sendTasks[i]);
    safePipe(getResults[i]);
    pid = fork();
    if (pid == ERROR) perrorExit("Error while creating slave process\n");
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
      perrorExit("execve() error");
    } else {
      close(sendTasks[i][READ]);
      close(getResults[i][WRITE]);
    }
  }
  return i;
}

static int minInt(int x, int y) { return x < y ? x : y; }

// https://www.tutorialspoint.com/unix_system_calls/_newselect.htm
// https://jameshfisher.com/2017/02/24/what-is-mode_t/
// https://man7.org/linux/man-pages/man2/mmap.2.html
// shared memory is stored in /dev/shm/results
