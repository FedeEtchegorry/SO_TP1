#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#define PID_BUFFER 10
#define ERROR (-1)
#define RW_MODE 0666

static void exitWithFailure(const char * errMsg); // can be a function for all files
int app_process_exists();



int main(int argc, char* argv[]){
   const int SIZE = 1024;
   const char* name = "MD5_VIEWER";

   int shm_fd;
   char* ptr;
   shm_fd = shm_open(name, O_RDONLY, RW_MODE );                                    //With this I create the shared memory. O_RDONLY means i will only read from a previous created shm
   if (shm_fd==ERROR)                                                                       //Error handling
       exitWithFailure("SHM_OPEN error");
   if(ftruncate(shm_fd, SIZE) == -1)
       exitWithFailure("FTRUNCATE error\n");
   ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);      //mapping of the shared memory
   if (ptr==(char*)-1)                                                                     //mapping error handling
       exitWithFailure("MMAP error");
   int i=0;
   while((ptr[i]!='\0' || app_process_exists())&(i<SIZE)) {                                 //NEED SEMAPHORES!!!!
        printf("%c", ptr[i++]);
   }
   if (shm_unlink(name)==ERROR)                                                             //Unlinkind from the share memory
       exitWithFailure("SHM_UNLINK error");
   return 0;
}


int app_process_exists(){                                                           //This allows to check if the app is still running
    char line[PID_BUFFER];
    FILE * command = popen("pidof ./app","r");
    if (command==NULL){
        exitWithFailure("POPEN error");
    }
    if (fgets(line,PID_BUFFER,command)==NULL){
        exitWithFailure("FGETS error");
    }
    pid_t pid = atoi(line);
    if (pid==0){
        exitWithFailure("ATOI error");
    }
    if (pclose(command)==ERROR){
        exitWithFailure("PCLOSE error" );
    };
    return pid;
}

static void exitWithFailure(const char * errMsg){
    perror(errMsg);
    exit(EXIT_FAILURE);
}