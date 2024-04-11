#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#define __USE_XOPEN_EXTENDED // for ftruncate
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define SHM_SIZE 1024
#define SHM_NAME "/results"
#define SLAVE_CMD "./slave"
#define RW_MODE 0666
#define ERROR -1
#define CHILD 0
#define FORK_QUANT 5
typedef int pipe_t[2];
enum { READ = 0, WRITE = 1 };

static void exitWithFailure(const char *errMsg); // can be a function for all files

int main(int argc, char const *argv[]) {
  argv++; // así arranco por los paths
  int fileQuant = argc - 1;
  if (fileQuant < 1)
    exitWithFailure("No files were passed\n");

  int shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, RW_MODE);
  if (shmFd == ERROR)
    exitWithFailure("Error while creating shm\n");

  if (ftruncate(shmFd, SHM_SIZE) == ERROR)
    exitWithFailure("Error while truncating shm\n");

  puts(SHM_NAME);
  sleep(5);

  char *buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  if (buf == MAP_FAILED)
    exitWithFailure("Error while mapping shm\n");

  int pid;
  pipe_t sendTasks[FORK_QUANT];
  pipe_t getResults[FORK_QUANT];

  int i;
  for (i = 0; i < FORK_QUANT; i++) {
    if (pipe(sendTasks[i]) == ERROR)
      exitWithFailure("Error while creating pipe\n");
    if (pipe(getResults[i]) == ERROR)
      exitWithFailure("Error while creating pipe\n");
    pid = fork();
    if (pid == ERROR)
      exitWithFailure("Error while creating slave process\n");
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

      char *argv[] = {SLAVE_CMD, NULL};
      char *envp[] = {NULL};
      execve(SLAVE_CMD, argv, envp);
      exitWithFailure("Error while calling slave process\n");
    } else {
      close(sendTasks[i][READ]);
      close(getResults[i][WRITE]);
      if(i < fileQuant)
        write(sendTasks[i][WRITE], argv[i], strlen(argv[i]));
    }
  }
  /*
  file descriptors deberían quedar:
  app = stdin, stdout, sendTasks[i][WRITE], getResults[i][READ] (i entre 0 y
  FORK_QUANT - 1) s0 = sendTasks[0][READ], getResults[0][WRITE] s1 =
  sendTasks[1][READ], getResults[1][WRITE] s2 = sendTasks[2][READ],
  getResults[2][WRITE]
  ...
  s(FORK_QUANT - 1) = sendTasks[FORK_QUANT - 1][READ], getResults[FORK_QUANT -
  1][WRITE]

  la comunicación sería:
  app por sendTask[i][WRITE] -> slave por sendTask[i][READ]

  slave por getResults[i][WRITE] -> app por getResults[i][READ]
  */

  fd_set rfds;
  int maxFd = getResults[0][READ];
  char *iniAddress = buf;
  int resultsCount = 0;
  int j;

  while (resultsCount < fileQuant) {
    FD_ZERO(&rfds); // preparo el rfds en cada iteración ya que el select es destructivo
    for (j = 0; j < FORK_QUANT; j++) {
      FD_SET(getResults[j][READ], &rfds);
      maxFd = getResults[j][READ] > maxFd ? getResults[j][READ] : maxFd;
    }
    FD_SET(READ, &rfds);

    select(maxFd + 1, &rfds, NULL, NULL, NULL);

    for (j = 0; j < FORK_QUANT; j++) { // cuando hay algo para leer, busco cuál es
      if (FD_ISSET(getResults[j][READ], &rfds)) {
        buf += read(getResults[j][READ], buf, SHM_SIZE - (buf - iniAddress)); // lo mando al buffer y aumento el puntero
        resultsCount++;
        if(i < fileQuant){
          write(sendTasks[i][WRITE], argv[i], strlen(argv[i]));
          i++;
        }
      }
    }
  }
  char endOfFile = EOF;
  for (i = 0; i < FORK_QUANT; i++) { 
    close(sendTasks[i][WRITE]); // closing the w-end, should lead to the r-end receiving EOF
    close(getResults[i][READ]);
  }

  if (munmap(buf, SHM_SIZE) == ERROR)
    exitWithFailure("Error while unmapping shm\n");

  if (close(shmFd) == ERROR)
    exitWithFailure("Error while closing shm\n");

  exit(EXIT_SUCCESS);
}

static void exitWithFailure(const char *errMsg) {
  fprintf(stderr, "%s", errMsg);
  exit(EXIT_FAILURE);
}

// https://www.tutorialspoint.com/unix_system_calls/_newselect.htm
// https://jameshfisher.com/2017/02/24/what-is-mode_t/
// https://man7.org/linux/man-pages/man2/mmap.2.html
// shared memory is stored in /dev/shm/results