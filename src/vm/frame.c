#include "vm/frame.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"

#include "vm/page.h"



void initialize_frame_table(void){
  list_init(&global_frame_table);
  lock_init(&global_frame_lock);

}

//This function will only work if the global frame lock has been aquired by its caller
void * try_evict(struct frame_entry* victim,int flag){

  struct sup_page_table_entry* page_table=victim->supplementry_PT;
  if(!victim->supplementry_PT->is_pinned) {
    struct thread *t = victim->owner;

    if(pagedir_is_accessed(t->pagedir,page_table->user_vaddr)){
      //Since we are evicting the frame, we want to make sure that it appears
      //to not have been accessed
      pagedir_set_accessed(t->pagedir, page_table->user_vaddr, false);

    }else{
      //If its dirty then we can swap it out or write it to its memory mapped file
      if(pagedir_is_dirty(t->pagedir,page_table->user_vaddr)){

        if(victim->supplementry_PT->for_mmap=true){
          //We write our victims frame to the file
          lock_acquire(&file_lock);
          file_write_at(page_table->file,victim->frame,page_table->file_read_bytes,page_table->file_offset);
          lock_release(&file_lock);
        }else{
          //If its a swap frame then we can swap it out
          //TODO implement swap_page_out
          //PANIC("OH NO I DONT KNOW HOW TO SWAP YET PLEASE HALP ME!!!!");

        }

      }
      page_table->is_loaded=false;
      list_remove(&victim->elem);
      pagedir_clear_page(t->pagedir,page_table->user_vaddr);
      palloc_free_page(victim->frame);
      free(victim);
      struct frame_entry* fresh_frame=palloc_get_page(flag);
      if(!fresh_frame){
        //PANIC("You filled up the swap space. You monster.");
      }
      return fresh_frame;
    }
  }
  return NULL;
}
struct frame_entry* fAlloc(struct sup_page_table_entry* curPage,int flag){
  void* new_frame=palloc_get_page(flag);
  lock_acquire(&global_frame_lock);

  while(!new_frame){
    printf("DEBUG: palloc failed, attempting to evict!\n");
    struct list_elem *e=list_begin(&global_frame_table);

    while(true) {

      struct frame_entry *victim = list_entry(e,struct frame_entry,elem);
      //We try to evict the current victim
      struct frame_entry * evicted_victim=try_evict(victim,flag);
      //If we successfuly evicited our victim then we set our
      //newframe to it and leave the loop
      if(evicted_victim){
        new_frame=evicted_victim;
        break;
      }
      //if we hit the end of the list
      //then we just go back to the start and repeat
      e=list_next(e);
      if(e==list_end(&global_frame_table)){
        e=list_begin(&global_frame_table);
      }
    }
  }
  //Now that we have a new frame we gotta add it to our frame table
  struct frame_entry* frame_to_add= malloc(sizeof(struct frame_entry));

  frame_to_add->frame=new_frame;
  frame_to_add->owner=thread_current();
  list_push_back(&global_frame_table,&frame_to_add->elem);
  lock_release(&global_frame_lock);
}
void fFree(struct frame_entry* curFrame){
  struct list_elem* e;
  lock_acquire(&global_frame_lock);
  //We just walk through our global frame table
  //and if we find a frame_entry which has the same frame as curFrame
  //we remove and free it
  for (e = list_begin(&global_frame_table); e != list_end(&global_frame_table); e = list_next(e)) {

    struct frame_entry* frame_to_remove=list_entry(e,struct frame_entry,elem);

    if(frame_to_remove->frame=curFrame){
      list_remove(e);
      free(frame_to_remove);
      palloc_free_page(curFrame);
      break;
    }
  }
  lock_release(&global_frame_lock);
}
