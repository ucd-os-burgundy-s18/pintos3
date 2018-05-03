#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"

struct lock file_lock;

bool isValidAddress(void* addr){
  if (addr != NULL && is_user_vaddr (addr))
  {

    return (pagedir_get_page (thread_current()->pagedir, addr) != NULL);

  }
  return false;
}
struct file_node {

    struct file* aFile;
    int fd;
    struct list_elem node;
    tid_t owner;

};
void exitWithError(){
  thread_current()->exit_status=-1;
  thread_exit();
}
void checkForBadArgs(struct intr_frame *f,int numArgs){
  if(!is_user_vaddr(f->esp+(4*numArgs))) {

    exitWithError();
  }

}

struct file_node* get_file_node(int fd){
  struct list* f_list=&thread_current()->fileList;
  struct list_elem* e;
  struct file_node* F;

  for (e = list_begin(f_list); e != list_end(f_list); e = list_next(f_list)) {
    //printf("Closed\n");
    struct file_node* F= list_entry(e,struct file_node, node);
    //printf("FD IS %i FFD is %i\n",fd,F->fd);
    if (fd==F->fd) {
      return F;
      //break;

    }
  }
  return NULL;

}

static void syscall_handler(struct intr_frame *);

void
syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");

  lock_init(&file_lock);
}

/*Placeholder system call, can be used for debug*/
void placeHolderSyscall(struct intr_frame *f){

  printf("SYSTEM CALL '%i' IS NOT IMPLEMENTED YET!!!!!!!!!!!!!\n",get_user(f->esp));
}

void haltSyscall(struct intr_frame *f) {

  shutdown ();
}

void exitSyscall(struct intr_frame *f){
  checkForBadArgs(f,1);
  int value = get_user(f->esp + 4);
  if(is_user_vaddr(f->esp + 4)) {
    //printf("VALUE IS %i\n", value);

    thread_current()->exit_status = value;
    thread_exit();
  }else{
    exitWithError();
  }
}

void execSyscall(struct intr_frame *f){






  unsigned long buffer_address = get_user(f->esp + 7);
  buffer_address = buffer_address * 256 + get_user(f->esp + 6);
  buffer_address = buffer_address * 256 + get_user(f->esp + 5);
  buffer_address = buffer_address * 256 + get_user(f->esp + 4);

  if(!is_user_vaddr(buffer_address)){
    exitWithError();
  }
  if(!isValidAddress((void*)buffer_address)){

    exitWithError();
  }

  //printf((char*)buffer_address);

  ///printf("eee\n");

  f->eax=process_execute(buffer_address);


}

void waitSyscall(struct intr_frame *f){
  tid_t id=get_user(f->esp + 4);
  f->eax=process_wait(id);
}





void createsyscall(struct intr_frame *f)
{

  unsigned long buffer_address = get_user(f->esp + 7);
  buffer_address = buffer_address * 256 + get_user(f->esp + 6);
  buffer_address = buffer_address * 256 + get_user(f->esp + 5);
  buffer_address = buffer_address * 256 + get_user(f->esp + 4);

  if(!isValidAddress((void*)buffer_address)){
    exitWithError();
  }
  if(buffer_address==0){
    //printf("NULL FILENAME\n");
    exitWithError();
    //return;
  }else {

  unsigned long initial_size = get_user(f->esp + 11);
  initial_size = initial_size * 256 + get_user(f->esp + 10);
  initial_size = initial_size * 256 + get_user(f->esp + 9);
  initial_size = initial_size * 256 + get_user(f->esp + 8);


    lock_acquire(&file_lock);


    bool success = filesys_create(buffer_address, initial_size);

    lock_release(&file_lock);
    f->eax = (int) success;
    //printf("CREATED\n");
    //return;
  }
}



void open(struct intr_frame *f)
{

  unsigned long buffer_address = get_user(f->esp + 7);
  buffer_address = buffer_address * 256 + get_user(f->esp + 6);
  buffer_address = buffer_address * 256 + get_user(f->esp + 5);
  buffer_address = buffer_address * 256 + get_user(f->esp + 4);
  //char file[128];
  if(!isValidAddress((void*)buffer_address)){
    exitWithError();
  }
  char* file=(char*)buffer_address;

  if(file[0]=='\0') {
    //printf("NULL INPUIT\n");
    f->eax=-1;
    return;
  }
  struct file_node* a_node = (struct file_node*)malloc(sizeof(struct file_node));
  if(!a_node){
    f->eax=-1;
    lock_release(&file_lock);

    return;
  }
  lock_acquire(&file_lock);
  struct file *afile = filesys_open(file);

  if(!afile)
  {
    //printf("BAD FILE\n");
    f->eax=-1;
    lock_release(&file_lock);
    return;
  }



  a_node->aFile = afile;
  a_node->fd = thread_current()->fd;
  a_node->owner=thread_current()->tid;
  thread_current()->fd++;

  list_push_back(&thread_current()->fileList, &a_node->node);

  lock_release(&file_lock);

  //return a_node->fd;
  f->eax=a_node->fd;

}

void closeSyscall(struct intr_frame *f) {
  int fd = get_user(f->esp + 4);
  if (fd < 2) {
    f->eax = -1;
    return;
  }
  lock_acquire(&file_lock);

  struct list_elem *e;
  struct list *f_list = &thread_current()->fileList;

  if (fd != -1) {
    struct file_node *F = get_file_node(fd);
    if(F!=NULL) {
      file_close(F->aFile);
      list_remove(&F->node);
      free(F);

    }

  } else {
    for (e = list_begin(f_list); e != list_end(f_list); e = list_next(f_list)) {
      struct file_node *F = list_entry(e,
      struct file_node, node);
      file_close(F->aFile);
      list_remove(&F->node);
      free(F);
    }
  }

  //printf("Closed\n");
  lock_release(&file_lock);

}


void readSyscall(struct intr_frame *f)
{

  //int readSyscall(int fd, void* buffer, unsigned size)

  int fd = get_user(f->esp + 4);
  unsigned size = get_user(f->esp + 15);
  size =size*256+ get_user(f->esp + 14);
  size =size*256+ get_user(f->esp + 13);
  size =size*256+ get_user(f->esp + 12);
  //printf("Reading size %i\n",size);

  //printf("Buffer start address: '%p'\n", start_of_buffer);

  unsigned long buffer_address = get_user(f->esp + 11);
  buffer_address = buffer_address * 256 + get_user(f->esp + 10);
  buffer_address = buffer_address * 256 + get_user(f->esp + 9);
  buffer_address = buffer_address * 256 + get_user(f->esp + 8);


  void* buffer=(void*)buffer_address;
  if(!isValidAddress(buffer)){
    //printf("Invalid address!\n");
    exitWithError();
  }

  //if(buffer[0]=='\0'){
   // exitWithError();
  //}

  //putbuf((char*)buffer,11);
  if(fd<2){
    f->eax=-1;
    return;
  }
  if(fd>thread_current()->fd){
    f->eax=-1;
    return;
  }


  if(fd == 0)
  {
    int j = 0;
    uint8_t* locBuffer = (uint8_t*) buffer;
    for(j = 0; j < size; ++j)
    {
      locBuffer[j] = input_getc();
    }
    f->eax=size;
    return;
  }

  lock_acquire(&file_lock);
  struct file_node *F=get_file_node(fd);




  struct aFile *aFile = F->aFile;

  if(!aFile)
  {

    lock_release(&file_lock);
    printf("BADD FILE\n");
    f->eax=-1;
    return;
  }
  //printf("Got file\n");
  int num_bytes = file_read(aFile, buffer, size);
  //printf("READ FILE\n");

  lock_release(&file_lock);
  f->eax=num_bytes;
  return;

}

void writeSyscall(struct intr_frame *f) {
  //Getting arguements from stack
  int fd = get_user(f->esp + 4);

  //unsigned size = get_user(f->esp + 12);
  unsigned size = get_user(f->esp + 15);
  size =size*256+ get_user(f->esp + 14);
  size =size*256+ get_user(f->esp + 13);
  size =size*256+ get_user(f->esp + 12);

  //printf("Buffer start address: '%p'\n", start_of_buffer);

  unsigned long buffer_address = get_user(f->esp + 11);
  buffer_address = buffer_address * 256 + get_user(f->esp + 10);
  buffer_address = buffer_address * 256 + get_user(f->esp + 9);
  buffer_address = buffer_address * 256 + get_user(f->esp + 8);

  void* buffer=(void*)buffer_address;
  if(!isValidAddress(buffer)){
    exitWithError();
  }
  if((fd<=0)||fd>thread_current()->fd){
    //f->eax=-1;
    //return;
    exitWithError();
  }
  if (fd == 1) {
    putbuf(buffer_address, size);

    f->eax=size;
    return;
  }
//printf("FD=%i\n",fd);
  if(fd<1||fd>thread_current()->fd){
    //Checks for invalid fd's
    //printf("FAIL\n");
    exitWithError();
  }

  lock_acquire(&file_lock);
  struct file_node *F=get_file_node(fd);




  ASSERT(F->fd==fd);

  struct file *aFile = F->aFile;

  if(!aFile)
  {

    lock_release(&file_lock);
    printf("BADD FILE\n");
    f->eax=-1;
    return;
  }

  //writing to the file itself

  int num_bytes = file_write (aFile, buffer,size);

  //After we write to the file we release the lock and
  //Setup our interupt frame to return to the user
  lock_release(&file_lock);
  f->eax=num_bytes;
  return;

}

//void seeksyscall(int fd, unsigned position)
void seeksyscall(struct intr_frame *f)
{
  int fd=get_user(f->esp + 4);

  unsigned position = get_user(f->esp + 11);
  position =position*256+ get_user(f->esp + 10);
  position =position*256+ get_user(f->esp + 9);
  position =position*256+ get_user(f->esp + 8);
  lock_acquire(&file_lock);

  struct list_elem *e;

  struct file_node *F;

  e = list_begin(&thread_current()->fileList);

  while( F->fd != fd ||  e != list_end(&thread_current()->fileList) ) {

      F = list_entry(e, struct file_node, node);

    e = list_next(e);
  }



  struct file *aFile = F->aFile;
  if(!aFile)
  {
    lock_release(&file_lock);
    //f->eax=-1;
    return;
  }

  file_seek(aFile, position);

  lock_release(&file_lock);

}



//bool removesyscall(const char* file)
bool removesyscall(struct intr_frame *f)
{

  unsigned long buffer_address = get_user(f->esp + 7);
  buffer_address = buffer_address * 256 + get_user(f->esp + 6);
  buffer_address = buffer_address * 256 + get_user(f->esp + 5);
  buffer_address = buffer_address * 256 + get_user(f->esp + 4);
  //char file[128];
  char* file=(char*)buffer_address;

  lock_acquire(&file_lock);

  bool check = filesys_remove(file);

  lock_release(&file_lock);

  return check;
}

//int filesizesyscall(int fd)
void filesizesyscall(struct intr_frame *f)
{
  int fd=get_user(f->esp+4);
  lock_acquire(&file_lock);



  struct file_node *F=get_file_node(fd);





  struct file *aFile = F->aFile;

  if(!aFile)
  {
    lock_release(&file_lock);
    f->eax=-1;
    return;

  }

  int size = file_length(aFile);

  lock_release(&file_lock);

  f->eax=size;
  return;

}

static void
syscall_handler(struct intr_frame *f) {
  /*SYSCALL LIST*/


  int (*p[14]) (void* sp);
  p[0]=haltSyscall;
  p[1]=exitSyscall;
  p[2]=execSyscall;
  p[3]=waitSyscall;
  p[4]=createsyscall;//Create
  p[5]=removesyscall;//Remove
  p[6]=open;//Open
  p[7]=filesizesyscall;//Filesize
  p[8]=readSyscall;//Read
  p[9]=writeSyscall;      //Write
  p[10]=seeksyscall;//seek
  p[11]=placeHolderSyscall;//tell
  p[12]=closeSyscall;//close


  void *stack_pointer = f->esp;

  //printf("Address is '%p'\n", stack_pointer);
  //printf("PHYS_BASE is '%p'\n", PHYS_BASE);


  if (is_user_vaddr(stack_pointer)) {


    int index=get_user(stack_pointer);
    //printf("RUNNING SYSCALL %i\n",index);
    if(index<0 || index>12){
      thread_current()->exit_status=-1;
      thread_exit();
    }
    p[index](f);
    return;

  }

  thread_exit();
}
