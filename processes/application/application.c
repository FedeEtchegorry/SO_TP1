#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#define __USE_XOPEN_EXTENDED
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <unistd.h>
#define SHM_SIZE 1024
#define SHM_NAME "/results"
#define SLAVE_CMD "./slave"
#define RW_MODE 0666
#define ERROR -1
#define CHILD 0
#define FORK_QUANT 5
typedef int pipe_t[2];
enum{READ=0, WRITE=1};

static void
exitWithFailure(const char *errMsg); // can be a function for all files

int main(int argc, char const *argv[]) {
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
  pipe_t getTasks[FORK_QUANT];

  for (int i = 0; i < FORK_QUANT; i++) {
    if (pipe(getTasks[i]) == ERROR)
      exitWithFailure("Error while creating pipe\n");
    pid = fork();
    if (pid == ERROR)
      exitWithFailure("Error while creating slave process\n");
    else if (pid == CHILD) {
      close(READ);
      dup(sendTasks[i][READ]);
      close(WRITE);
      dup(getTasks[i][WRITE]);
      close(sendTasks[i][READ]);
      close(sendTasks[i][WRITE]);
      close(getTasks[i][READ]);
      close(getTasks[i][WRITE]);

      char * argv[] = {NULL};
      char * envp[] = {NULL};
      execve(SLAVE_CMD, argv, argc);
      exitWithFailure("Error while calling slave process\n");
    }
    else {
      close(sendTasks[i][READ]);
      close(getTasks[i][WRITE]);
    }
  }
  fd_set rfds;
  FD_ZERO(&rfds);
  int count = 0;
  int maxFd = getTasks[0][READ];
  for (int i = 0; i < FORK_QUANT; i++) { 
    FD_SET(getTasks[i][READ], &rfds);
    maxFd = getTasks[i][READ] > maxFd ? getTasks[i][0] : maxFd;
  }

  int inc;
  char * iniAddress = buf;
  
  while (count < fileQuant) {

    select(maxFd + 1, &rfds, NULL, NULL, NULL);

    for (int i = 0; i < FORK_QUANT; i++) {
      if (FD_ISSET(getTasks[i][READ], &rfds)) {
        
        inc = read(getTasks[i][READ], buf, SHM_SIZE - (buf - iniAddress));
        if(inc > 0){
          buf += inc;
          count++;
        }
        write(sendTasks[i][WRITE], argv[i], sizeof(argv[i]));
        
      }
      FD_SET(getTasks[i][0], &rfds);
    }
  }

  if (munmap(buf, SHM_SIZE) == ERROR)
    exitWithFailure("Error while unmapping shm\n");

  if (close(shmFd) == ERROR)
    exitWithFailure("Error while closing shm\n");

  exit(0);
}

static void exitWithFailure(const char *errMsg) {
  perror(errMsg);
  exit(EXIT_FAILURE);
}

// https://www.tutorialspoint.com/unix_system_calls/_newselect.htm
// https://jameshfisher.com/2017/02/24/what-is-mode_t/
// https://man7.org/linux/man-pages/man2/mmap.2.html
// shared memory is stored in /dev/shm/results