#include <globals.h>
#include <stdio.h>
#include <utils.h>

#define PID_BUFFER 10
#define ERROR (-1)
// #define RW_MODE 0666
#define SHM_NAME_BUF_SIZE 100
// #define RESULT_BUF_SIZE 100

// int app_process_exists();
int readFromShm(char* buf, sem_t* sem);

int main(int argc, char* argv[]) {
  // Lo hago con getchar para evitar tener que reemplazar el \n
  char shmName[SHM_NAME_BUF_SIZE];
  int shmNameLen = 0;
  char c;
  while ((c = getchar()) != '\n' && c != EOF) shmName[shmNameLen++] = c;
  shmName[shmNameLen] = 0;
  printf("From view shm name: %s\n", shmName);

  int shmFd = safeShmOpen(shmName, O_RDONLY, 0 /* RW_MODE */);
  // safeFtruncate(shmFd, SHM_SIZE);
  char* const shmBuf = safeMmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shmFd, 0);
  char* shmBufCurrent = shmBuf;

  printf("sem name: %s\n", SEM_NAME);
  sem_t* enabledForRead = safeSemOpenRead(SEM_NAME);

  int resultLength = 0;
  do {
    printf("Reading result...\n");
    resultLength = readFromShm(shmBufCurrent, enabledForRead);
    shmBufCurrent += resultLength;
  } while (resultLength > 0);
  // while ((shmBuf[i] != '\0' || app_process_exists()) & (i < SIZE)) { // NEED SEMAPHORES!!!!
  //   printf("%c", shmBuf[i++]);
  // }

  if (shm_unlink(shmName) == ERROR) perrorExit("shm_unlink() error on view");

  return 0;
}

int readFromShm(char* buf, sem_t* sem) {
  int len = 0;
  while (buf[len] != '\n' && buf[len] != EOF) {
    printf("%c", buf[len++]);
  }
  sem_wait(sem);
  return buf[len] == EOF ? -1 : ++len;
}

// int app_process_exists() { // This allows to check if the app is still running
//   char line[PID_BUFFER];
//   FILE* command = popen("pidof ./app", "r");
//   if (command == NULL) {
//     exitWithFailure("POPEN error");
//   }
//   if (fgets(line, PID_BUFFER, command) == NULL) {
//     exitWithFailure("FGETS error");
//   }
//   pid_t pid = atoi(line);
//   if (pid == 0) {
//     exitWithFailure("ATOI error");
//   }
//   if (pclose(command) == ERROR) {
//     exitWithFailure("PCLOSE error");
//   };
//   return pid;
// }
