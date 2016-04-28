#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct argument{
    char * argument;
    uint32_t size;
    int offset;
}arg;

#endif /* userprog/process.h */
