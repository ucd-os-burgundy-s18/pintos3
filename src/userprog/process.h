#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"
tid_t process_execute (const char *file_name);
struct thread* find_by_tid(tid_t tid,struct thread* cur);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct arguments {
    struct thread *parent;
    char *args;
    bool success;
    struct semaphore child_spawn_lock; /*Prevents the parent from running until the child has compleatly started or failed to start*/

};

/*Created on the fly in process_wait()
 *
 * */
struct childStatus{
    struct semaphore blocker;
    int exit_code;
};

#endif /* userprog/process.h */
