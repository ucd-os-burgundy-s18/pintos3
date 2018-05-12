#include <string.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/page.h"



//alternate install page that is no differnt from the one in process.c
//Its here because its 3:50 am as I type this
static bool
install_page1(void *upage, void *kpage, bool writable) {
  struct thread *t = thread_current();

  // Verify that there's not already a page at that virtual
  //   address, then map our page there.
  return (pagedir_get_page(t->pagedir, upage) == NULL
          && pagedir_set_page(t->pagedir, upage, kpage, writable));
}


bool extend_stack(void* user_vaddr){
  //We start by creating our new page table entry
  struct sup_page_table_entry *new_stack_page=malloc(sizeof(struct sup_page_table_entry));
  //Since we are creating a new page, we want its address to be at the bottom
  //of page relitive to the user virtural address
  ASSERT(new_stack_page);
  new_stack_page->user_vaddr=pg_round_down(user_vaddr);
  new_stack_page->is_loaded=true;
  new_stack_page->is_writible=true;
  new_stack_page->is_pinned=true;

  //We create a frame that our page is gonna map too
  void* new_frame=fAlloc(PAL_USER,new_stack_page);
  //and check its validity
  if(!new_frame){
    free(new_stack_page);
    print("DEBUG: FRAME FAILED TO ALLOCATE!!!!\n");
    return false;
  }
  if(!install_page1(user_vaddr,true)){
    free(new_stack_page);
    fFree(new_frame);
    print("DEBUG: INSTALL PAGE FAILED!!!!\n");
  }
  if(intr_context()){
    new_stack_page->is_pinned= false;
  }
  return list_insert(&thread_current()->page_list,new_stack_page->elem);
}

//All this function does is walk through the threads page list and returns the page with
//the same address as our rounded down user_vaddr
struct sup_page_table_entry* get_pte_from_user(void* user_vaddr){
  struct list_elem *e;

  for (e = list_begin(&thread_current()->page_list); e != list_end(&thread_current()->page_list); e = list_next(e)) {

    struct sup_page_table_entry cur_page=list_entry(e,struct sup_page_table_entry,elem);

    if(cur_page->user_vaddr==pg_round_down(user_vaddr)){

      return cur_page;
    }
  }
  return NULL;
}


//TODO: implement these fuckers

bool page_load(struct sup_page_table_entry* pt){

}
bool mmap_load(struct sup_page_table_entry* pt){

}
bool swap_load(struct sup_page_table_entry* pt){

}
bool file_load(struct sup_page_table_entry* pt){

}
bool pt_add_file(struct file* file,int32_t offset,uint8_t * upage,uint32_t file_read_bytes,uint32_t file_zero_bytes){

}
bool pt_add_mmap(struct file* file,int32_t offset,uint8_t * upage,uint32_t file_read_bytes,uint32_t file_zero_bytes){

}

