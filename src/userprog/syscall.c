#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  void*stack_pointer=f->esp;
  printf("Address is '%p'\n",stack_pointer);
  printf("PHYS_BASE is '%p'\n",PHYS_BASE);

  char buffer[PGSIZE];
  if(is_user_vaddr(stack_pointer)){
    printf("This is in user space\n");
  }
  //int buffer[PGSIZE];
  //hex_dump (PHYS_BASE-PGSIZE-1,buffer, PGSIZE, true);
 //for(int i =0; i<250; ++i){
  //if(get_user((uintptr_t) stack_pointer - (uintptr_t) i)==-1){
   //print("SEGFAULT\n");
   //break;
  //}
 // printf("%c",get_user(stack_pointer+i));
 //}
 thread_exit ();
}
