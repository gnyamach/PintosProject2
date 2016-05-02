#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <src/lib/user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

#User added

#include "vaddr.h"
#include "../threads/thread.h"

#define VADD_USER_BOTTOM =((void *) 0x08048000)
static void check_valid
struct lock file_lock;

void
syscall_init (void){
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  printf ("system call!\n");
  thread_exit ();
}

# Additions

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault
occurred. pg 26 */
static int
get_user (const uint8_t *uaddr)
{
  if(!is_user_vaddr(uaddr)){
    return -1;
  }
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
  : "=&a" (result) : "m" (*uaddr));
  return result;
}
/* Writes BYTE to user address UDST.
UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  if (!is_user_vaddr(udst)){
    return false;
  }
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
  : "=&a" (error_code), "=m" (*udst) : "r" (byte));
  return error_code != -1;
}

bool create(const char * file){
  lock_acquire(&file_lock);
  bool success = filesys_create(file);
  lock_release(&file_lock);
  return success;
}

bool remove(const char * file){
  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

int open(const char * file){
   lock_acquire(&file_lock);
   struct file * f = filesys_open(file);
   if(!f){
     lock_release(&file_lock);
     return TID_ERROR;
   }

   struct file * ofile = filesys_open(file);
   if(ofile == NULL){
     return -1;
   }
   struct file_elem * felem = malloc(sizeof(struct file_elem));
   felem->file_name = file;
   felem->file=ofile;
   felem->fd = thread_current()->fd;
   thread_current()->fd++;
   list_push_back(&thread_current()->file_list,&felem->elem);
   lock_release(&file_lock);
   return felem->fd;
}

int filesize(int fd){
    lock_acquire(&file_lock);
    struct file_elem * f;
    f= get_file(fd);
    if(f == NULL){
        lock_release(&file_lock);
        return 0;
    }
    int l = file_length(f);
    lock_release(&file_lock);
   return l;
}

int read(int fd, void * buffer, unsigned size){
    int bytes = 0;
    if(fd ==1){
        //file input is file out so bad
        return -1;
    }

    lock_acquire(&file_lock);
    if(fd == 0) {
        //file system input so read from keyboard
         int fsize = 0;
         uint8_t *fbuf = (uint8_t *) buffer;
         for (i = 0; i < size; i++) {
            fsize += 1;
            fbuf[i] = input_getc();
         }
        lock_release(&file_lock);
        return fsize;

    }else{
        //regular file so read from file
        struct file_elem *f = get_file(fd);
        if(f == NULL){
            lock_release(&file_lock);
            return -1;
        }else{
            bytes = file_read(f->file,size);
        }
    }
    lock_release(&file_lock);
    return bytes;

}

int write(int fd,const void * buffer, unsigned size){
    int bytes = 0;
    lock_acquire(&file_lock);
    if(fd == 1){
        //write to standard out
        putbuf(buffer,size);
        lock_release(&file_lock);
        return size;
    }else{
        //write to file
        struct file_elem * f = get_file(fd);
        if(f == NULL){
            lock_release(&file_lock);
            return -1;
        }

        bytes = file_write(f->file,buffer,size);
    }
    lock_release(&file_lock);
    return bytes;
}

void seek(int fd, unsigned position){
    lock_acquire(&file_lock);
    struct file_elem * f = get_file(fd);
    if(f != NULL){
        file_seek(f->file,position);
    }
    lock_release(&file_lock);
}

unsigned tell(int fd){
   off_t byte;
   lock_aquire(&file_lock);
   struct file_elem * f = get_file(fd);
   byte = file_tell(f);
   lock_release(&file_lock);
   return byte;
}

void close(int fd){
   lock_acquire(&file_lock);
    //get file
    struct file_elem * f = get_file(fd);
    struct thread * t = thread_current();

    //remove and close for current thread
    list_remove(f->elem);
    file_close(f->file);
    free(f->file_name);
    free(f);

   lock_release(&file_lock);
}

int wait(pid_t pid){
  return process_wait(pid);
}

void exit(int status){
  struct thread * t = thread_current();
  if(thread_is_alive(t->parent)){
      t->cp->status = status;
  }

  thread_exit();
}

void halt(){
  shutdown_power_off();
}

void check_valid_ptr(const void *vaddr){
  if(vaddr <VADD_USER_BOTTOM){
    exit(ERROR);
  }
}

struct file_elem *  get_file(int fd){
    struct thread * t = thread_current();
    struct list_elem * e;
    for(e = list_begin(&t->file_list); e != list_end(&t->file_list);e = list_next(e)){
        struct file_elem * fp = list_entry(e,struct file_elem,elem);
        if(fd == fp->fd){
            return fp;
        }
    }
    return NULL;
}