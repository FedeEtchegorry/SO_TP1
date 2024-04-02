#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


#define PID_ERROR (-1)
#define PARENT_PID 0                                             //The pid 0 refers to the parent process but the parent pid is stored by the child
#define EXECVE_ERROR (-1)

void md5sum_caller(char* path);

int main(int argc, char* argv[]) {
    for (int i = 1; argv[i]!=NULL; i++) {                                       //The slave will process a number of files sended by the master
        md5sum_caller(argv[i]);
    }
}

void md5sum_caller(char* path){
    pid_t pid = fork();
    if (pid==PID_ERROR){                                                        //Fork error handling
        perror("Fork call couldn´t be completed\n");
        exit(EXIT_FAILURE);
    } else if (pid==PARENT_PID){                                                //In the child process
        char* args[3]={"md5sum",path, NULL};                        //Every path is considered valid
        if (execve("./md5sum", args, NULL)==EXECVE_ERROR) {     //Execve error handling
            perror("Process md5sum couldn´t be executed");
            exit(EXIT_FAILURE);
        }
    } else{                                                                     //In the parent process
        int status;
        waitpid(pid, &status, 0);                                //Wait for md5sum to finish processing
    }
}

//TODO: implementar shared memory para recibir output de md5sum y armar el buffer que le va a llegar al master: