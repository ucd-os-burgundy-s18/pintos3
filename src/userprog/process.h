#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
struct thread* find_by_tid(tid_t tid);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct arguments{
    tid_t process_to_run;
    char* args;
};
#endif /* userprog/process.h */
