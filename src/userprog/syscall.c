#include <stdio.h>

#include <syscall-nr.h>

#include "userprog/syscall.h"

#include "userprog/pagedir.h"

#include "threads/interrupt.h"

#include "threads/thread.h"

#include "threads/vaddr.h"

#include "devices/shutdown.h"

#include "lib/syscall-nr.h"

#include "threads/synch.h"

#include "filesys/filesys.h"

#include "filesys/file.h"

#include "threads/malloc.h"





static void syscall_handler (struct intr_frame *);

void exit (int status);

int write (int fd, const void *buffer, unsigned size);

void halt(void);

tid_t exec (const char *cmd_line);

int wait (int pid);

bool create (const char *file, unsigned initial_size);

bool remove (const char *file);

int open (const char *file);

int filesize (int fd);

int read (int fd, void *buffer, unsigned size);

void seek (int fd, unsigned position);

unsigned tell (int fd);

void close (int fd);

struct child_element *get_thread_child(tid_t tid);

struct file_descriptor* get_file_from_fd(int given_fd);





void

syscall_init (void) 

{

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);

}



static void is_valid_pointer(void *user_pointer) {

  struct thread *t = thread_current();

  // It is a valid pointer if it's not a pointer to kernel virtual address space, if it's not a null pointer, and if it doesn't point

  //                                                          to unmapped virtual memory, which is when pagedir_get_page returns NULL

  if (!is_user_vaddr(user_pointer) || user_pointer == NULL) {

    exit(-1);

  }

  void *page = pagedir_get_page(t->pagedir, user_pointer);

  if (page == NULL) {

    exit(-1);

  }

}



static void

syscall_handler (struct intr_frame *f UNUSED) 

{



  is_valid_pointer((void *) f->esp);

  void *stack = f->esp; // Assign stack to a variable for easier pointer math

  int syscall = *((int *) f -> esp);

  stack += 4;

  

  switch (syscall) {


    case SYS_HALT:

    {

      halt();

      break;

    }



    case SYS_EXIT:

    {

      is_valid_pointer((void *)stack);

      int prog_status = *(int *)stack;

      stack += 4;

      // printf("\nProg status is %d\n", prog_status);

      exit(prog_status);

      f->eax = prog_status;

      break;

    }



    case SYS_WRITE: // mike

    {

      is_valid_pointer((void *)stack);

      int prog_fd = *(int *)stack;

      //printf("\nProg_fd is %d\n", prog_fd);

      stack += 4;

      is_valid_pointer((void *)stack);

      char *prog_buffer = *(int *)stack;  

      //printf("\nProg_buffer is %d\n", prog_buffer);

      stack += 4;

      is_valid_pointer((void *)stack);

      unsigned prog_size = *(unsigned *)stack;

      //printf("\nProg_size is %d\n", prog_size);

      stack += 4;      

      f->eax = write(prog_fd, (const void *) prog_buffer, prog_size); // Need to store result in the eax register

      break;

    }



    case SYS_EXEC: // mike

    {

      is_valid_pointer((void *)stack);

      const char *cmd_line = (*((const char**)(stack)));

      stack += 4;

      f->eax = exec(cmd_line);

      break;

    }



    case SYS_WAIT:

    {
      is_valid_pointer((void *)stack);

      int p = (*((int*)(stack)));

      stack += 4;

      f->eax = wait(p);

      break;

    }



    case SYS_CREATE: // mike

    {

      is_valid_pointer((void *)stack);

      const char *file = (*((const char**)(stack)));

      // printf("\n\nfile is %s", file);

      stack += 4;

      is_valid_pointer((void *)stack);

      unsigned initial_size = (*((unsigned*)(stack)));


      // printf("\n\ninitial size is %u", initial_size);

      stack += 4;

      f->eax = create(file, initial_size);

      break;

    }



    case SYS_REMOVE:

    {
      is_valid_pointer((void *)stack);

      const char *file_to_remove = (*((const char**)(stack)));

      stack += 4;

      f->eax = remove(file_to_remove);

      break;

    }



    case SYS_OPEN: // mike

    {

      is_valid_pointer((void *)stack);

      const char *file = (*((const char**)(stack)));

      // printf("\n\nfile is %s", file);

      stack += 4;

      f->eax = open(file);

      break;
      
    }



    case SYS_FILESIZE:

    {
      is_valid_pointer((void *)stack);

      int prog_fd = (*((int*)(stack)));

      stack += 4;

      f->eax = filesize(prog_fd);

      break;
    }



    case SYS_READ: // mike

    {
      is_valid_pointer((void *)stack);
      int fd = (*((int*)(stack)));
      stack += 4;

      is_valid_pointer((void *)stack);
      int buffer = (*((void**)(stack)));
      stack += 4;

      is_valid_pointer((void *)stack);
      unsigned size = (*((unsigned*)(stack)));
      stack += 4;

      f->eax = read(fd, buffer, size);

      break;

    }



    case SYS_SEEK:

    {
      is_valid_pointer((void *)stack);
      int prog_fd = *(int *)stack;
      stack += 4;

      is_valid_pointer((void *)stack);
      unsigned pos_arg = (unsigned)stack;
      stack += 4;

      seek(prog_fd, pos_arg);

      break;
    }



    case SYS_TELL:

    {
      is_valid_pointer((void *)stack);
      int prog_fd = *(int *)stack;
      stack += 4;

      f->eax = tell(prog_fd);

      break;
    }



    case SYS_CLOSE:

    {
      is_valid_pointer((void *)stack);
      int prog_fd = *(int *)stack;
      stack += 4;

      close(prog_fd);

      break;
    }

  }



  //printf ("system call!\n");

  //thread_exit ();

}



void halt (void) {

  shutdown_power_off();

}



void exit (int status){

  //retrieve name of running program

  char *current_thread_name = thread_current()->name;

  printf ("%s: exit(%d)\n", current_thread_name, status);


  //terminate running program


  thread_exit();

}



int write (int fd, const void *buffer, unsigned size){

  // printf("\nInside write\n");

  //per pintos doc, fd argument of 1 means write to console
  is_valid_pointer(buffer);
  lock_acquire(&file_lock);

  if (fd == 1){

    //call putbuf as per stdio.h: void putbuf (const char *, size_t);
    putbuf(buffer, size);
    lock_release(&file_lock);

    return size; //return number of bytes written
  } else if (fd == 0) { // indicates stdin

    lock_release(&file_lock);
    return 0;

  } else{

    struct file_descriptor* temp_fd = get_file_from_fd(fd);

    if (temp_fd == NULL) {
      return -1;
    }

    int result = file_write(temp_fd->actual_file, buffer, size);
    lock_release(&file_lock);

    return result;

  }

}



// Runs the executable whose name is given in cmd_line, passing any given arguments, and returns the new process's program id (pid). 

// Must return pid -1, which otherwise should not be a valid pid, if the program cannot load or run for any reason. 

// Thus, the parent process cannot return from the exec until it knows whether the child process successfully loaded its executable. You must use appropriate synchronization to ensure this. 

tid_t exec (const char *cmd_line){
  is_valid_pointer(cmd_line);

  
  struct thread *cur = thread_current();
  tid_t the_pid = -1; // will be -1 if process doesn't load or run correctly

  lock_acquire(&file_lock);
  the_pid = process_execute(cmd_line);
  lock_release(&file_lock);

  return the_pid;
}



// Waits for a child process pid and retrieves the child's exit status. 

// see Doc at 3.3.4 for detailed info

int wait (int pid){

  return process_wait(pid);

}



// Creates a new file called file initially initial_size bytes in size. Returns true if successful, false otherwise. 

// Creating a new file does not open it: opening the new file is a separate operation which would require a open system call. 

bool create (const char *file, unsigned initial_size){
  is_valid_pointer(file);
  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return success;

}



// Deletes the file called file. Returns true if successful, false otherwise. 

// A file may be removed regardless of whether it is open or closed, and removing an open file does not close it.

// See Removing an Open File (3.4.2) in doc

bool remove (const char *file){
  is_valid_pointer(file);
  lock_acquire(&file_lock);
  bool result = filesys_remove(file);
  lock_release(&file_lock);
  return result;

}



// Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened. 

// see Doc at 3.3.4 for detailed info

int open (const char *file){
  is_valid_pointer(file);
  size_t filename_length = strlen(file);
  if (file == NULL || filename_length == 0){
    return -1;
  }

  int value = -1;
  lock_acquire(&file_lock);

  struct file *f = filesys_open(file);

  lock_release(&file_lock);

  if (f == NULL) {
    
    return -1;
  } else {
    struct file_descriptor *ff = malloc (sizeof(struct file_descriptor));
    ff->actual_file = f;
    thread_current()->num_of_descriptors++;
    value = thread_current()->num_of_descriptors;
    ff->fd = value;
    list_push_back(&thread_current()->file_list, &ff->fd_elem);
    return value;
  }

}



// Returns the size, in bytes, of the file open as fd. 

int filesize (int fd){

  struct file_descriptor* temp_fd = get_file_from_fd(fd);

  lock_acquire(&file_lock);
  int result = file_length(temp_fd->actual_file);
  lock_release(&file_lock);
  return result;
}



// Reads size bytes from the file open as fd into buffer. Returns the number of bytes actually read (0 at end of file), 

// or -1 if the file could not be read (due to a condition other than end of file). Fd 0 reads from the keyboard using input_getc(). 

int read (int fd, void *buffer, unsigned size){
  
  is_valid_pointer(buffer);
  if (buffer == NULL) {
    return -1;
  }

  int result = -1; // default return value
  if (fd == 0) {
    return (int) input_getc();
  } else if (fd == 1 || list_empty(&thread_current()->file_list)) {
    return 0;
  } else {
    struct file_descriptor *f = get_file_from_fd(fd);
    if (f == NULL) {
      return -1;
    }
    struct file *the_file = f->actual_file;
    lock_acquire(&file_lock);
    result = file_read(the_file, buffer, size);
    lock_release(&file_lock);
    if (result != 0 && result < (int)size) {
      // file_read: "Returns the number of bytes actually read, which may be less than SIZE if end of file is reached."
      return -1;
    }
  }
  return result;

}



// Changes the next byte to be read or written in open file fd to position, expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.) 

// A seek past the current end of a file is not an error. A later read obtains 0 bytes, indicating end of file. A later write extends the file, filling any unwritten gap with zeros.

void seek (int fd, unsigned position){
  struct file_descriptor *f = get_file_from_fd(fd);

  if (f == NULL) {
    return;
  } else {
    struct file *the_file = f->actual_file;
    lock_acquire(&file_lock);
    file_seek(the_file, position);
    lock_release(&file_lock);
  }

lock_acquire(&file_lock);
// file_sys(file_name, position);
lock_release(&file_lock);

}





// Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning of the file. 

unsigned tell (int fd){
  struct file_descriptor* temp_fd = get_file_from_fd(fd);

  if (temp_fd == NULL) {
    return -1;
  }

  lock_acquire(&file_lock);
  unsigned result = file_tell(temp_fd->actual_file);
  lock_release(&file_lock);

  return result;

}



// Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, as if by calling this function for each one. 

void close (int fd){
  if (fd == 0 || fd == 1){
  exit(-1);
  }
  if (fd > 1000){
  exit(-1);
  }

  int temp_fd_counter = 0;
  int given_fd = fd;
  struct thread *cur = thread_current();
  struct list_elem *e;
  for (e = list_begin (&cur->file_list); e != list_end (&cur->file_list); e = list_next (e)) {
  struct file_descriptor *f = list_entry(e, struct file_descriptor, fd_elem);
  if (f->fd == given_fd) {
      temp_fd_counter++;
    }
  }

  if (temp_fd_counter == 0){
  exit(-1);
}


  struct file_descriptor* temp_fd = get_file_from_fd(fd);
  lock_acquire(&file_lock);
  file_close(temp_fd->actual_file);
  lock_release(&file_lock);
}



struct child_element *get_thread_child(tid_t tid) {
  struct thread *cur = thread_current();
  struct list_elem *e;
  for (e = list_begin (&cur->children); e != list_end (&cur->children); e = list_next (e)) {
    struct child_element *c = list_entry(e, struct child_element, child_elem);
    if (c->pid_child == tid) {
      return c;
    }
  }
  return NULL; // no results
}


struct file_descriptor* get_file_from_fd(int given_fd) {
  struct thread *cur = thread_current();
  struct list_elem *e;
  for (e = list_begin (&cur->file_list); e != list_end (&cur->file_list); e = list_next (e)) {
    struct file_descriptor *f = list_entry(e, struct file_descriptor, fd_elem);
    if (f->fd == given_fd) {
      return f;
    }
  }
  return NULL; // no results
}