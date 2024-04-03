#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


//#define PID_ERROR (-1)
//#define PARENT_PID 0                                             //The pid 0 refers to the parent process but the parent pid is stored by the child
//#define EXECVE_ERROR (-1)



#define BUFFER_SIZE 256

void md5sum_caller(char* path, char* buffer);

int main(int argc, char* argv[]) {
    for (int i = 1; argv[i]!=NULL; i++) {                                       //The slave will process a number of files sended by the master
        char buffer[BUFFER_SIZE]={'\0'};
        md5sum_caller("slave", buffer);
        buffer[strlen(buffer)-1]='\0';                                       //Remove the newline char after the fgets
        printf("%s - PROCESS PID %d",buffer, getpid());
    }
}

void md5sum_caller(char* path, char* buffer) {
    char md5sum_command[BUFFER_SIZE]="md5sum ";                                                 //The md5sum call to be made
    strcpy(md5sum_command+ strlen(md5sum_command), path );                  //Add the file path to the md5sum_command
    FILE *fp = popen(md5sum_command, "r");                              //Popen makes the fork call and creates the pipe, r means reading mode
    if( fp == NULL ){                                                                   //check Popen failure
        perror("POPEN couldn´t be created successfully");
        exit(EXIT_FAILURE);
    }
    if(fgets(buffer, BUFFER_SIZE, fp)==NULL) {                           //Fgets fills the buffer with the md5sum result
        perror("FGETS was unsuccessful to read the data into the buffer");         //Error handling
        exit(EXIT_FAILURE);
    }
}


//void md5sum_caller(char* path){
//    pid_t pid = fork();
//    if (pid==PID_ERROR){                                                        //Fork error handling
//        perror("Fork call couldn´t be completed\n");
//        exit(EXIT_FAILURE);
//    } else if (pid==PARENT_PID){                                                //In the child process
//        char* args[3]={"md5sum",path, NULL};                        //Every path is considered valid
//        if (execve("./md5sum", args, NULL)==EXECVE_ERROR) {     //Execve error handling
//            perror("Process md5sum couldn´t be executed");
//            exit(EXIT_FAILURE);
//        }
//    } else{                                                                     //In the parent process
//        int status;
//        waitpid(pid, &status, 0);                                //Wait for md5sum to finish processing
//    }
//}
