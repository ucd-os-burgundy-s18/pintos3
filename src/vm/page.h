//
// Created by pintos on 5/12/18.
//

#ifndef PINTOS_PAGE_H
#define PINTOS_PAGE_H
#include <stdbool.h>
#include <stdint.h>
struct sup_page_table_entry{
    unit32_t* user_vaddr;

    bool for_swap;
    bool for_mmap;
    bool is_loaded;
    bool is_pinned;
    size_t swap_id;
    struct list_elem elem;


    struct file* file;
    size_t file_offset;
    size_t file_read_bytes;
    size_t file_zero_bytes;


};

struct sup_page_table_entry* get_pte_from_user(void* user_vaddr);
void initialize_page_table(struct list* table);
void delete_page_table(struct list* table);

bool page_load(struct sup_page_table_entry* pt);
bool mmap_load(struct sup_page_table_entry* pt);
bool swap_load(struct sup_page_table_entry* pt);
bool file_load(struct sup_page_table_entry* pt);
bool pt_add_file(struct file* file,int32_t offset,uint8_t * upage,uint32_t file_read_bytes,uint32_t file_zero_bytes);
bool pt_add_mmap(struct file* file,int32_t offset,uint8_t * upage,uint32_t file_read_bytes,uint32_t file_zero_bytes);
bool extend_stack(void* user_vaddr);



#endif //PINTOS_PAGE_H
