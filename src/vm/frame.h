#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/synch.h"
#include "vm/page.h"
#include <stdbool.h>
#include <stdint.h>
#include <list.h>
struct list global_frame_table;
struct lock global_frame_lock;
struct lock global_evictor_lock;
struct frame_entry {
    void* base_address;
    struct thread* owner;
    struct list_elem elem;
    struct sup_page_table_entry* supplementry_PT;
    void* frame;


};
void initialize_frame_table(void);
struct frame_entry* fAlloc(struct sup_page_table_entry* curPage,int flag);//Allocates a new locked frame
void fFree(struct frame_entry* curFrame);//Frees the frame

#endif /* vm/frame.h */