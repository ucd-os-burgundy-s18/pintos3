#include "vm/page.h"
#include <string.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

#include "userprog/syscall.h"


//check_expected ([<<'EOF']);


//alternate install page that is no differnt from the one in process.c
//Its here because its 3:50 am as I type this

bool
install_page1(void *upage, void *kpage, bool writable) {
  struct thread *t = thread_current();


  // Verify that there's not already a page at that virtual
  //   address, then map our page there.
  return (pagedir_get_page(t->pagedir, upage) == NULL
          && pagedir_set_page(t->pagedir, upage, kpage, writable));
}


bool extend_stack(void* user_vaddr){
  if(!user_vaddr){
    printf("BAD USER ADDRESS!\n");
    return false;
  }

  //printf("input address is  %p\n",user_vaddr);
  //ASSERT(user_vaddr==pg_round_down(user_vaddr));

  if ( (size_t) (PHYS_BASE - pg_round_down(user_vaddr)) > (1 << 23)) {
    //printf("DKJADHKJAAD\n");
    return false;
  }
  //printf("DEBUG: EXTENDING STACK\n");
  //We start by creating our new page table entry
  struct sup_page_table_entry *new_stack_page=malloc(sizeof(struct sup_page_table_entry));
  //printf("New address is %p\n",new_stack_page);
  if(!new_stack_page){
    return false;
  }


  //Since we are creating a new page, we want its address to be at the bottom
  //of page relitive to the user virtural address
  //printf("Page allocated\n");
  ASSERT(new_stack_page);
  new_stack_page->user_vaddr=pg_round_down(user_vaddr);
  //printf("Created page with address %p\n",pg_round_down(user_vaddr));
  new_stack_page->is_loaded=true;
  new_stack_page->is_writable=true;
  new_stack_page->is_pinned=true;

  //We create a frame that our page is gonna map too
  void* new_frame=fAlloc(PAL_USER,new_stack_page);
  //and check its validity

  if(!new_frame){
    free(new_stack_page);
    printf("DEBUG: FRAME FAILED TO ALLOCATE!!!!\n");
    return false;
  }
  //printf("installing page at adddress %p\n",user_vaddr);
  if(!install_page1(new_stack_page->user_vaddr,new_frame,true)){
    free(new_stack_page);
    fFree(new_frame);
    printf("DEBUG: INSTALL PAGE FAILED!!!\n");
    return false;
  }
  //printf("page installed\n");
  if(intr_context()){
    new_stack_page->is_pinned= false;
  }
  //printf("final insert\n");
  list_push_back(&thread_current()->page_list,&new_stack_page->elem);
  //printf("done\n");
  return true;
}

//All this function does is walk through the threads page list and returns the page with
//the same address as our rounded down user_vaddr
struct sup_page_table_entry* get_pte_from_user(void* user_vaddr){
  struct list_elem *e;
  enum intr_level old_level;
  //printf("DEBUG: Checking %p with\n",pg_round_down(user_vaddr));
  old_level = intr_disable ();
  for (e = list_begin(&thread_current()->page_list); e != list_end(&thread_current()->page_list); e = list_next(e)) {

    struct sup_page_table_entry* cur_page=list_entry(e,struct sup_page_table_entry,elem);
    //printf("DEBUG: Checking %p with list with %p\n",pg_round_down(user_vaddr),cur_page->user_vaddr);
    if(cur_page->user_vaddr==pg_round_down(user_vaddr)){
      intr_set_level (old_level);
      return cur_page;
    }
  }
  intr_set_level (old_level);
  //printf("Failed to find\n");
  return NULL;
}

void setFile(struct sup_page_table_entry* page,struct file* file,int32_t offset,uint8_t * upage, uint32_t file_read_bytes,uint32_t file_zero_bytes,bool writable){
  page->user_vaddr=upage;
  page->file=file;
  page->file_offset=offset;
  page->file_read_bytes=file_read_bytes;
  page->file_zero_bytes=file_zero_bytes;
  page->is_loaded=false;
  page->is_pinned=false;
}


bool pt_add_file(struct file* file,int32_t offset,uint8_t * upage,uint32_t file_read_bytes,uint32_t file_zero_bytes,bool writible,size_t debug){
  //printf("DEBUG: %p is set to lazily load a file when accesed\n",upage);
  struct sup_page_table_entry *new_page=malloc(sizeof(struct sup_page_table_entry));

  if(!new_page) {
    printf("BOGAS\n");
    return false;
  }
  //printf("DEBUG: ADDING UPAGE %p \n",pg_round_down(upage));

  //printf("ID %p\n",debug);
  ASSERT(upage==pg_round_down(upage));
  //setFile(new_page,file,offset,upage,file_read_bytes,file_zero_bytes,writible);
  new_page->debugID=debug;
  new_page->user_vaddr=upage;
  new_page->file=file;
  new_page->file_offset=offset;
  new_page->file_read_bytes=file_read_bytes;
  new_page->file_zero_bytes=file_zero_bytes;
  new_page->is_loaded=false;
  new_page->is_pinned=false;
  new_page->file_offset=offset;
  //printf("page parameters set!\n");
  new_page->is_writable=writible;
  new_page->for_file=true;

  new_page->is_loaded=false;
  new_page->is_pinned=false;
  //printf("NEW UPAGE IS %p\n",new_page->user_vaddr);
  //file_load(new_page);
  list_push_back (&thread_current()->page_list, &new_page->elem);

  //printf("ADDED FILE!\n");
  return true;
}

bool pt_add_mmap(struct file* file,int32_t offset,uint8_t * upage,uint32_t file_read_bytes,uint32_t file_zero_bytes, bool writable){
 struct sup_page_table_entry *new_page=malloc(sizeof(struct sup_page_table_entry));

  if(!new_page) {
    return false;
  }
  //printf("DEBUG: ADDING UPAGE %p \n",pg_round_down(upage));

  //printf("ID %p\n",debug);
  ASSERT(upage==pg_round_down(upage));
  //setFile(new_page,file,offset,upage,file_read_bytes,file_zero_bytes,writible);
  new_page->debugID=0;
  new_page->user_vaddr=upage;
  new_page->file=file;
  new_page->file_offset=offset;
  new_page->file_read_bytes=file_read_bytes;
  new_page->file_zero_bytes=file_zero_bytes;
  new_page->is_loaded=false;
  new_page->is_pinned=false;
  new_page->file_offset=offset;
  //printf("page parameters set!\n");
  new_page->is_writable=writable;
  new_page->for_mmap=true;

  new_page->is_loaded=false;
  new_page->is_pinned=false;
  //printf("NEW UPAGE IS %p\n",new_page->user_vaddr);
  //file_load(new_page);
  list_push_back (&thread_current()->page_list, &new_page->elem);

  //printf("ADDED FILE!\n");
  return true;


}


bool page_load(struct sup_page_table_entry* pt){
  pt->is_pinned=true;
  if(pt->is_loaded){
    //printf("DEBUG: %p is already loaded\n");
    return true;
  }
  //printf("LOADING\n");
  if(pt->for_file){
    return file_load(pt);
  }
  if(pt->for_swap){
    printf("Swap\n");
    return NULL;
  }
  if(pt->for_mmap){
    printf("mmap\n");
    return NULL;
  }
  //printf("NOT LOADED\n");
  return false;
}
bool mmap_load(struct sup_page_table_entry* pt){

}
bool swap_load(struct sup_page_table_entry* pt){

}
bool file_load(struct sup_page_table_entry* pt){
  //printf("ID IS %i\n",pt->debugID);

  enum palloc_flags flags=PAL_USER;
  if(pt->file_read_bytes==0){
    //printf("SETTING TO ZERO\n");
    flags |= PAL_ZERO;

  }
  uint8_t *file_frame=NULL;
  file_frame=fAlloc(pt,flags);

  //printf("DEBUG: %p is now getting a frame at %p mapped to it\n",pt->user_vaddr,file_frame);
  if(file_frame==NULL){
    //printf("BOGAS\n");
    return false;
  }

  if(pt->file_read_bytes) {
    //printf("Has bytes\n");
    lock_acquire(&file_lock);


    file_seek(pt->file, pt->file_offset);
    if (file_read(pt->file, file_frame,pt->file_read_bytes) != (int) pt->file_read_bytes) {
      lock_release(&file_lock);
      fFree(file_frame);
      printf("File read error\n");
      file_seek(pt->file, 0);
      return false;
    }


    lock_release(&file_lock);
    //printf("Loaded setting memory\n");
    memset(file_frame + pt->file_read_bytes, 0, pt->file_zero_bytes);
    //printf("Memory set!\n");
  }
 // printf("installing page!\n");
  if(!install_page1(pt->user_vaddr,file_frame,pt->is_writable)){
    fFree(file_frame);
    //printf("DEBUG: Failed to install page\n");
    return false;
  }
  pt->is_loaded=true;
  //printf("DEBUG: %p has been successfuly loaded!\n",pt->user_vaddr);
  return true;
}

