#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// General purpose functions

int minInt(int x, int y);

// Wrapper functions used for managing errors

void perrorExit(const char* errMsg);
void exitWithFailure(const char* errMsg);
ssize_t safeRead(int fd, void* buf, size_t nbytes);
ssize_t safeWrite(int fd, void* buf, size_t nbytes);
void safePipe(int pipedes[2]);
void safeClose(int fd);
void safeDup(int fd);
int safeShmOpen(const char* name, int oflag, mode_t mode);
void safeFtruncate(int fd, __off_t length);
char* safeMmap(void* addr, size_t len, int prot, int flags, int fd, __off_t offset);
void safeMunmap(void* addr, size_t len);
sem_t* safeSemOpenCreate(const char* name, mode_t permissions, int initialValue);
sem_t* safeSemOpenRead(const char* name);
void safeSemClose(sem_t* sem);

#endif
