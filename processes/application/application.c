#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>
#define __USE_XOPEN_EXTENDED
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define SHM_SIZE 1024
#define SHM_NAME "/results"
#define RW_MODE 0666 
#define ERROR -1

static void exitWithFailure(const char * errMsg); // can be a function for all files

int main(int argc, char const *argv[]) {
  int fileQuant = argc - 1;
  if(fileQuant < 1)
    exitWithFailure("No files were passed\n");

  int shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, RW_MODE);
  if(shmFd == ERROR)
    exitWithFailure("Error while creating shm\n");

  if(ftruncate(shmFd, SHM_SIZE) == ERROR)
    exitWithFailure("Error while truncating shm\n");

  puts(SHM_NAME);
  sleep(5);

  char * buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  if(buf == MAP_FAILED)
    exitWithFailure("Error while mapping shm\n");

  if(munmap(buf, SHM_SIZE) == ERROR)
    exitWithFailure("Error while unmapping shm\n");

  if(close(shmFd) == ERROR)
    exitWithFailure("Error while closing shm\n");

  // int iniFileQuant = fileQuant/10 + 1;
  // fd_set rfds;
  // int maxFd;
  
  return 0;
}

static void exitWithFailure(const char * errMsg){
  perror(errMsg);
  exit(EXIT_FAILURE);
}

// https://www.tutorialspoint.com/unix_system_calls/_newselect.htm 
// https://jameshfisher.com/2017/02/24/what-is-mode_t/ 
// https://man7.org/linux/man-pages/man2/mmap.2.html
// shared memory is stored in /dev/shm/results