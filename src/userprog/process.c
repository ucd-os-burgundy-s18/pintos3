#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


static thread_func start_process
NO_RETURN;

static bool load(const char *cmdline, void (**eip)(void), void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */




tid_t
process_execute(const char *file_name) {
  if (strlen(file_name) >= 128) {
    return TID_ERROR;
  }
  char fn_copy[128];
  //char* fn_args;
  char program_name[128];
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */

  //fn_args=palloc_get_page(0);

  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy(fn_copy, file_name, 128);
  //printf("Input is '%s'\n", fn_copy);
  struct thread* cur=thread_current();

  int argc = 0;
  char *cur_arguement;
  char *token;
  char *str1;
  int j = 0;
  //This first loop checks the number of arguements
  //so we can push argc to the stack
  str1 = fn_copy;

  token = strtok_r(str1, " ", &cur_arguement);
  strlcpy(program_name, token, 128);
  strlcpy(fn_copy, file_name, 128);//Strtok messes up fn_copy from file_name

  //Everything here is static, even the stuff that gets passed to the child
  //To ensure we dont die before them we use a semaphore to block the parent
  //to give the child time to setup

  struct arguments cc;
  struct arguments *cur_args = &cc;
  cur_args->parent = thread_current();
  cur_args->args = fn_copy;

  sema_init(&cur_args->child_spawn_lock,0);
  cur_args->success=false;
  /* Create a new thread to execute PROGRAM_NAME.
   * For whatever reason it did not like passing the arguements struct in to start_process
   * via thread_create so I just decided to pass in the entire string and reparse it later*/
  tid = thread_create(program_name, PRI_DEFAULT, start_process, cur_args);

  if (tid == TID_ERROR) {
    //palloc_free_page(fn_copy);
    //palloc_free_page(program_name);
    tid=-1;
  }else {

    sema_down(&cur_args->child_spawn_lock);
    // printf("Woke up\n");
    if(cur_args->success==false){
      tid=-1;
      //printf("BOGAS\n");
    }
    //free(cur_args->args);
    //free(cur_args);

    //printf("Exited sema down\n");
  }



  //palloc_free_page (argv);
  return tid;
}


/* A thread function that loads a user process and starts it
   running. */
static void
start_process(void *in_args) {
  //printf("ENTERED START_PROCESS!\n");

  struct arguments *args_struct = (struct arguments *) in_args;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset(&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load(args_struct->args, &if_.eip, &if_.esp);

  //palloc_free_page(args_struct->args);//We clear out the textual arguements to save spac
  args_struct->args=NULL;
  /* If load failed, quit. */

  if (!success) {
    //printf("Failed to start\n");


    thread_current()->exit_status=-1;
    //printf("Unlocking parent\n");
    args_struct->success=false;
    sema_up(&args_struct->child_spawn_lock);
   // printf("Unlocked\n");
    thread_current()->failed_to_spawn=true;
    thread_exit();


  }

  //We initialize the current threads parent and synronazation varibles
  args_struct->success=true;
  //setup_parent(args_struct->parent);
  struct thread *cur = thread_current();
  cur->parent = args_struct->parent;
  //printf("Pushing child with id %i\n",cur->tid);
  list_push_back(&cur->parent->children, &cur->child_elem);

  //We add ourselves to the parents children list
  //And unblock the parent
  sema_up(&args_struct->child_spawn_lock);
  thread_yield();//Once we are ready to switch to user mode we let the parent finish and cleanup first.


  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED();
}
/* HELPERS FOR SYNCRONAZATION OF PROCESSES*/

//Used to search a children list by thread id.
struct thread *find_by_tid(tid_t tid,struct thread* cur) {

  struct thread* child=NULL;
  struct list *children = &cur->children;
  if (!list_empty(children)) {

    struct list_elem *e;


    //printf("ENTERED SEARCH\n");
    for (e = list_begin(children); e != list_end(&cur->children); e = list_next(e)) {
      //printf("ENTERED SEARCH1\n");
      child = list_entry(e,struct thread, child_elem);
      //printf("Comparing child id: %i to input %i\n",child->tid,tid);
      if (child->tid == tid) {
        //printf("GOT IT\n");
        break;

      }
    }
  }//else{
    //printf("LIST EMPTY\n");
  //}
  //printf("Returning\n");
  return child;
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait(tid_t child_tid) {
  //printf("ENTERED WAIT\n");
  struct thread* cur=thread_current();
  struct thread* child=find_by_tid(child_tid,cur);
  int exit_code=0;

  if (child != NULL) {
    cur->p_waiting_on=child_tid;
    /*
     * The childStatus struct contains a semaphore which we use to block the parent.
     * When the child dies, it calls sema_up() on the semaphore.
     *
     */
    struct childStatus* child_life=(struct childStatus*) malloc(sizeof(struct childStatus));
    sema_init(&child_life->blocker,0);
    child->p_waiter=child_life;

    sema_down(&child_life->blocker);
    exit_code=child_life->exit_code;

    free(child_life);
  }else{
    return -1;//If the ID dosent exist in the threads children list we return -1
  }

  return exit_code;
}

/* Free the current process's resources. */
void
process_exit(void) {

  struct thread *cur = thread_current();
  uint32_t *pd;
  if(!cur->failed_to_spawn) {
    printf("%s: exit(%d)\n", cur->name, cur->exit_status);
    enum intr_level old_level;
    //printf("afhkjahfjkafa\n");

    old_level = intr_disable ();
    /* Destroy the current process's page directory and switch back
       to the kernel-only page directory. */

    pd = cur->pagedir;
    if (pd != NULL) {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate(NULL);
      pagedir_destroy(pd);
    }
    /*We remove ourselves from our parents children list, and wake up the parent
     *if they are waiting on us we wake them up
     */
    if (cur->exe != NULL) {
      //printf("UNLOCKIGN EXE\n");
      file_allow_write(cur->exe);
      file_close(cur->exe);

      cur->exe = NULL;
    }
    //printf("Is it even working???\n");
    struct thread *t;
    if (&cur->parent) {
      //printf("Thread has parrent\n");
      struct thread *parent = &cur->parent;


      if (!list_empty(&cur->parent->children)) {

        list_remove(&cur->child_elem);

      }
      if (cur->p_waiter != NULL) {

        cur->p_waiter->exit_code = cur->exit_status;
        sema_up(&cur->p_waiter->blocker);

      }
    }
    if(!list_empty(&cur->children)){
      //printf("ENTERED SEARCH1\n");
      struct list_elem* e;
      struct thread* child=NULL;


      for (e = list_begin(&cur->children); e != list_end(&cur->children); e = list_next(e)) {
        //printf("ENTERED SEARCH1\n");
        child = list_entry(e,struct thread, child_elem);
        child->parent=NULL;


      }
    }

    intr_set_level (old_level);
  }//else{

  //}


}


/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate(void) {
  struct thread *t = thread_current();

  /* Activate thread's page tables. */
  pagedir_activate(t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */

struct Elf32_Ehdr {
    unsigned char e_ident[16];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
};

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack(void **esp, int argc, char **argv);

static bool validate_segment(const struct Elf32_Phdr *, struct file *);

static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes,
                         bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load(const char *cmdline, void (**eip)(void), void **esp) {
  //printf("aaa\n");
  char **argv = palloc_get_page(0);
  int argc = 0;
  char *scratch_space;
  char *token;
  char *str1;
  int j = 0;
  bool fail=false;
  //This first loop checks the number of arguements
  //so we can push argc to the stack
  for (j = 0, str1 = cmdline;; j++, str1 = NULL) {
    token = strtok_r(str1, " ", &scratch_space);
    if (token == NULL) {
      argc = j;
      break;
    }
    argv[j] = token;
  }
  //printf("loading\n");
  struct thread *t = thread_current();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;

  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create();
  if (t->pagedir == NULL)
    goto done;
  process_activate();

  /* Open executable file. */
  file = filesys_open(argv[0]);
  if (file == NULL) {
    printf("load: %s: open failed\n", argv[0]);

    return false;
    goto done;
  }
  file_deny_write(file);


  /* Read and verify executable header. */
  if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp(ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof(struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) {
    printf("load: %s: error loading executable\n", argv[0]);
    file_allow_write(file);
    return false;
    goto done;
  }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) {
    struct Elf32_Phdr phdr;

    if (file_ofs < 0 || file_ofs > file_length(file))
      goto done;
    file_seek(file, file_ofs);

    if (file_read(file, &phdr, sizeof phdr) != sizeof phdr)
      goto done;
    file_ofs += sizeof phdr;
    switch (phdr.p_type) {
      case PT_NULL:
      case PT_NOTE:
      case PT_PHDR:
      case PT_STACK:
      default:
        /* Ignore this segment. */
        break;
      case PT_DYNAMIC:
      case PT_INTERP:
      case PT_SHLIB:
        goto done;
      case PT_LOAD:
        if (validate_segment(&phdr, file)) {
          bool writable = (phdr.p_flags & PF_W) != 0;
          uint32_t file_page = phdr.p_offset & ~PGMASK;
          uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
          uint32_t page_offset = phdr.p_vaddr & PGMASK;
          uint32_t read_bytes, zero_bytes;
          if (phdr.p_filesz > 0) {
            /* Normal segment.
               Read initial part from disk and zero the rest. */
            read_bytes = page_offset + phdr.p_filesz;
            zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE)
                          - read_bytes);
          } else {
            /* Entirely zero.
               Don't read anything from disk. */
            read_bytes = 0;
            zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
          }
          if (!load_segment(file, file_page, (void *) mem_page,
                            read_bytes, zero_bytes, writable))
            goto done;
        } else
          goto done;
        break;
    }
  }

  /* Set up stack. */
  if (!setup_stack(esp, argc, argv))
    goto done;

  /* Start address. */
  *eip = (void (*)(void)) ehdr.e_entry;

  success = true;

  done:
  if(fail){
    file_allow_write(file);
    thread_current()->exit_status=-1;
    return false;
  }
  //palloc_free_page(argv);
  thread_current()->exe=file;
  /* We arrive here whether the load is successful or not. */

  return success;
}

/* load() helpers. */

static bool install_page(void *upage, void *kpage, bool writable);


//void push(void* kpage,uint8_t* ofs,void*buffer,size_t size);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment(const struct Elf32_Phdr *phdr, struct file *file) {
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length(file))
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;

  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment(struct file *file, off_t ofs, uint8_t *upage,
             uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
  ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT(pg_ofs(upage) == 0);
  ASSERT(ofs % PGSIZE == 0);

  file_seek(file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) {
    /* Calculate how to fill this page.
       We will read PAGE_READ_BYTES bytes from FILE
       and zero the final PAGE_ZERO_BYTES bytes. */
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    /* Get a page of memory. */
    uint8_t *kpage = palloc_get_page(PAL_USER);
    if (kpage == NULL)
      return false;

    /* Load this page. */
    if (file_read(file, kpage, page_read_bytes) != (int) page_read_bytes) {
      palloc_free_page(kpage);
      return false;
    }
    memset(kpage + page_read_bytes, 0, page_zero_bytes);

    /* Add the page to the process's address space. */
    if (!install_page(upage, kpage, writable)) {
      palloc_free_page(kpage);
      return false;
    }

    /* Advance. */
    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    upage += PGSIZE;
  }
  return true;
}

/*decrements *esp by size(moving to the next part in the stack)
 * and then copys the contents of buffer into esp
 */
void push(void *kpage, unsigned int *ofs, void *buffer, size_t size) {
  //printf("Before\n");
  char temp_buffer[size];
  //hex_dump(kpage+*ofs-size,kpage+*ofs-size,size, true);
  //hex_dump((unsigned int)(ofs-size),kpage,size, true);
  //printf("Old esp is '%p'\n",kpage+*ofs);
  *ofs -= size;
  //printf("new esp is '%p'\n",kpage+*ofs);
  //printf("Pushing '%s' with size '%i' to address '%p'\n",buffer,size,kpage+*ofs);
  memcpy(kpage + *ofs, buffer, size);
  //hex_dump(kpage+*ofs,kpage+*ofs,size, true);

  //*(kpage+*ofs)=5;
}


/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack(void **esp, int argc, char **argv) {
  //printf("Setting up stack\n");
  uint8_t *kpage;
  uint8_t *upage;
  bool success = false;
  uint32_t addresses[argc + 1];//When pushing arguements we save their addresses;
  kpage = palloc_get_page(PAL_USER | PAL_ZERO);

  if (kpage != NULL) {
    upage = ((uint8_t *) PHYS_BASE) - PGSIZE;
    success = install_page(upage, kpage, true);

    if (success) {


      *esp = PHYS_BASE;

      //*esp = PHYS_BASE - 12;
    } else {
      palloc_free_page(kpage);
    }
  }


  unsigned int ofs = PGSIZE;
  // - sizeof(uint8_t);

  //Now that we have our stack setup we gotta push our values to it
  //size_t offset = 0;
  for (int i = 0; i < argc; ++i) {
    size_t arg_size = strlen(argv[i]) + 1;
    //Inner loop used for counting the size of each arguement




    //We push our addresses
    //printf("Pushing argument '%s'\n",argv[i]);
    push(kpage, &ofs, argv[i], arg_size);
    addresses[i] = (unsigned int) (*esp) - ((unsigned int) PGSIZE - (unsigned int) (ofs));

    //addresses[i]=*esp-(unsigned int)PGSIZE-(unsigned int)(ofs);
    //printf("Done pushing\n");
    //hex_dump(kpage-PGSIZE,buffer,PGSIZE, true);
    //offset -= size_rounded;//We move our offset to the next position on the stack
  }
  //char test=0;
  //push(kpage,&test,argv[i], sizeof());
  //hex_dump(*esp,buffer,PHYS_BASE-*esp, true);
  /*Now that we have the contents of our arguements pushed in the stack, and their addresses on the
   * stack saved, we can word aligh the stack pointer*/
  //*esp=(void*)ROUND_DOWN((unsigned long)*esp,4);
  unsigned int i = ((unsigned int) (ofs)) % 4;
  ofs -= i;
  /*Now that we are aligned we can start pushing pointers to argv
   *We start with a null terminator for the entire argv
   */
  uint32_t null = 0;/*
 * Yes I know passing in a refrence to an int and de refrencing it in the function
 * Seems like extra work but it saves the time of having to make another push() function
  */
  push(kpage, &ofs, &null, sizeof(uint32_t));
  /*We then go in reverse order and push the addresses of each argv, rounding down each time*/
  for (int i = argc - 1; i >= 0; --i) {
    uint32_t cur_address = addresses[i];
    //push(esp,&cur_address, sizeof(cur_address));
    push(kpage, &ofs, &cur_address, sizeof(uint32_t));
  }
  /*We push the address of our new array on the stack in this case it is the last value of esp*/
  uint32_t start_of_argv = (unsigned int) (*esp) - ((unsigned int) PGSIZE - (unsigned int) (ofs));
  //*esp=(void*)ROUND_DOWN((unsigned long)*esp,4);
  //push(esp,&start_of_argv, 1);
  push(kpage, &ofs, &start_of_argv, sizeof(uint32_t));
  /*we then push argc*/
  push(kpage, &ofs, &argc, sizeof(uint32_t));
  //ofs -= sizeof(void *);
  /*and finaly our return address*/
  uint32_t zero = 0;
  push(kpage, &ofs, &zero, sizeof(uint32_t));
  //We change *esp back to its origional value

  //*esp=oldesp;


  //and then subtract our offset.
  unsigned int ofs2 = (unsigned int) PGSIZE - (unsigned int) (ofs);

  *esp = (unsigned int) (*esp) - ofs2;

  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page(void *upage, void *kpage, bool writable) {
  struct thread *t = thread_current();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page(t->pagedir, upage) == NULL
          && pagedir_set_page(t->pagedir, upage, kpage, writable));
}
