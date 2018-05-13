#include "vm/frame.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "threads/vaddr.h"
#include "vm/page.h"



void initialize_frame_table(void){
  list_init(&global_frame_table);
  lock_init(&global_frame_lock);

}

//This function will only work if the global frame lock has been aquired by its caller
void * try_evict(struct frame_entry* victim,enum palloc_flags flag){

  //struct sup_page_table_entry* page_table=victim->supplementry_PT;
  //printf("victim is %p\n",victim);
  //printf("victim PT %p\n",victim->supplementry_PT);
  if(!victim->supplementry_PT->is_pinned) {
    printf("IS NOT PINNED\n");

    struct thread *t = victim->owner;

    if(pagedir_is_accessed(t->pagedir,victim->supplementry_PT->user_vaddr)){
      //Since we are evicting the frame, we want to make sure that it appears
      //to not have been accessed
      pagedir_set_accessed(t->pagedir, victim->supplementry_PT->user_vaddr, false);
      printf("set accesed\n");
      return NULL;
    }

      //If its dirty then we can swap it out or write it to its memory mapped file
      if(pagedir_is_dirty(t->pagedir,victim->supplementry_PT->user_vaddr)||victim->supplementry_PT->for_swap){

        if(victim->supplementry_PT->for_mmap){
          printf("MMUP\n");
          //We write our victims frame to the file
          lock_acquire(&file_lock);
          file_write_at(victim->supplementry_PT->file,victim->frame,victim->supplementry_PT->file_read_bytes,victim->supplementry_PT->file_offset);
          lock_release(&file_lock);
        }else{
          printf("SWAP\n");
          //If its a swap frame then we can swap it out
          //TODO implement swap_page_out
          PANIC("OH NO I DONT KNOW HOW TO SWAP YET PLEASE HALP ME!!!!");

        }

      }
      //printf("Finalizing eviction\n");
      victim->supplementry_PT->is_loaded=false;
      list_remove(&victim->elem);
      pagedir_clear_page(t->pagedir,victim->supplementry_PT->user_vaddr);
      //printf("Victim is %p\n",victim->frame);
      palloc_free_page(victim->frame);
      free(victim);
      /*
      struct frame_entry* fresh_frame=palloc_get_page(flag);
      if(!fresh_frame){
        PANIC("You filled up the swap space. You monster.");
      }else {
        printf("Successfuly evicited\n");
      }*/
    printf("Successfuly evicited\n");
      return palloc_get_page(flag);

  }
 // printf("entered loop\n");

  return NULL;
}

void* fAlloc(struct sup_page_table_entry* curPage,enum palloc_flags flag){
  if(!(flag & PAL_USER)) {
    printf("NOT FOR KERNAL\n");
    return NULL;
  }
  //printf("PAGE VADDR IS %p\n",curPage->user_vaddr);
  void* new_frame=palloc_get_page(flag);


  while(!new_frame){


    lock_acquire(&global_frame_lock);
    struct list_elem *e=list_begin(&global_frame_table);
    struct frame_entry * evicted_victim=NULL;
    //printf("DEBUG: palloc failed, attempting to evict!\n");

    while(true) {

      struct frame_entry *victim = list_entry(e,struct frame_entry,elem);
      if(victim){
        if(victim->owner) {
          printf("ROTATED\n");
          evicted_victim = try_evict(victim, flag);
        }
      }
      //printf("ROTATED\n");
      //printf("%p is the victims address and %p is its pages address\n",victim,victim->supplementry_PT->user_vaddr);
      //printf("OK\n");

      //We try to evict the current victim


      //If we successfuly evicited our victim then we set our
      //newframe to it and leave the loop
      if(evicted_victim&&evicted_victim!=NULL){

        //printf("Got a valid frame! %p\n",evicted_victim);
        //printf("Rounded is!        %p\n",pg_round_down(evicted_victim));
        new_frame=evicted_victim;
        //printf("Eviction successful\n");
        //new_frame=palloc_get_page(flag);

        lock_release(&global_frame_lock);
        break;
      }
      //if we hit the end of the list
      //then we just go back to the start and repeat
      //printf("NEXT\n");
      e=list_next(e);

      if(e==list_end(&global_frame_table)){
        e=list_begin(&global_frame_table);
      }

    }
  }

  //printf("Got a valid frame! %p\n",new_frame);
  //Now that we have a new frame we gotta add it to our frame table
  lock_acquire(&global_frame_lock);
  struct frame_entry* frame_to_add=malloc(sizeof(struct frame_entry));

  frame_to_add->frame=new_frame;
  frame_to_add->supplementry_PT=curPage;
  frame_to_add->owner=thread_current();

  //printf("Frame created for thread %s with frame address of %p\n", thread_current()->name,new_frame);

  //printf("Adding to list\n");
  list_push_back(&global_frame_table,&frame_to_add->elem);
  //printf("Added!\n");
  lock_release(&global_frame_lock);
  //printf("frame address is %i\n",frame_to_add->user_vaddr);
  //printf("%p is the new file frames spt\n",frame_to_add->supplementry_PT);
  return new_frame;
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
