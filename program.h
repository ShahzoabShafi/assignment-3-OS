#pragma once

#include <stdbool.h>
#include <stddef.h>

struct Program {
    char *name;
    char *path;
    size_t line_count;
    size_t page_count;
    int *page_table;
    bool delete_on_reset;
    struct Program *next;
};

struct Program *get_or_create_program(const char *filename, bool *created);
void reset_program_registry(void);

size_t program_line_count(const struct Program *program);
const char *program_name(const struct Program *program);

int program_load_initial_pages(struct Program *program, size_t page_limit);
int program_load_page_on_fault(struct Program *program, size_t page_number);
int program_get_frame_for_page(const struct Program *program, size_t page_number);
const char *program_get_line(struct Program *program, size_t logical_pc, int *frame_index);
void program_invalidate_page(struct Program *program, size_t page_number);
