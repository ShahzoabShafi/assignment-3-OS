#pragma once

#include <stddef.h>

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 18
#endif

#ifndef VAR_STORE_SIZE
#define VAR_STORE_SIZE 10
#endif

#define PAGE_SIZE 3

struct Program;

void mem_init(void);
void mem_reset_frame_store(void);
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

size_t mem_get_frame_store_size(void);
size_t mem_get_var_store_size(void);
size_t mem_get_frame_count(void);

int frame_store_find_free_frame(void);
int frame_store_select_lru_victim(void);
void frame_store_touch(int frame_index);
const char *frame_store_get_line(int frame_index, size_t offset);

void frame_store_evict_frame(
    int frame_index,
    struct Program **victim_program,
    size_t *victim_page_number,
    char *victim_lines[PAGE_SIZE],
    size_t *victim_line_count
);

void frame_store_write_frame(
    int frame_index,
    struct Program *program,
    size_t page_number,
    char *page_lines[PAGE_SIZE],
    size_t line_count
);
