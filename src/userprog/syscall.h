#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"

void syscall_init (void);

struct child_element *get_thread_child(tid_t tid);

struct file_descriptor* get_file_from_fd(int given_fd);

struct file_descriptor {
    struct list_elem fd_elem;
    struct file *actual_file;
    int fd;
};

struct lock file_lock;

#endif /* userprog/syscall.h */
