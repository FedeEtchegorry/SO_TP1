#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// General purpose functions

int minInt(int x, int y);

// Wrapper functions used for managing errors

void perrorExit(const char* errMsg);
void exitWithFailure(const char* errMsg);
size_t safeRead(int fd, void* buf, size_t nbytes);
size_t safeWrite(int fd, void* buf, size_t nbytes);
void safePipe(int pipedes[2]);
void safeClose(int fd);
void safeDup(int fd);

#endif
