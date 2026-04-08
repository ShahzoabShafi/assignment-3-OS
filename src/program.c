#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "program.h"
#include "shell.h"
#include "shellmemory.h"

static struct Program *program_registry = NULL;

static void free_program(struct Program *program) {
    if (program->delete_on_reset) {
        unlink(program->path);
    }
    free(program->name);
    free(program->path);
    free(program->page_table);
    free(program);
}

static size_t count_program_lines(const char *path) {
    FILE *script = fopen(path, "rt");
    char linebuf[MAX_USER_INPUT];
    size_t line_count = 0;

    if (!script) {
        return (size_t)(-1);
    }

    while (fgets(linebuf, sizeof(linebuf), script)) {
        line_count++;
    }

    fclose(script);
    return line_count;
}

static int read_program_page(
    const struct Program *program,
    size_t page_number,
    char *page_lines[PAGE_SIZE],
    size_t *line_count
) {
    FILE *script = fopen(program->path, "rt");
    char linebuf[MAX_USER_INPUT];
    size_t first_line = page_number * PAGE_SIZE;

    if (!script) {
        return -1;
    }

    for (size_t i = 0; i < first_line; ++i) {
        if (!fgets(linebuf, sizeof(linebuf), script)) {
            fclose(script);
            return -1;
        }
    }

    *line_count = 0;
    for (size_t i = 0; i < PAGE_SIZE; ++i) {
        if (!fgets(linebuf, sizeof(linebuf), script)) {
            break;
        }
        page_lines[i] = strdup(linebuf);
        (*line_count)++;
    }

    fclose(script);
    return 0;
}

static void free_page_lines(char *page_lines[PAGE_SIZE]) {
    for (size_t i = 0; i < PAGE_SIZE; ++i) {
        free(page_lines[i]);
        page_lines[i] = NULL;
    }
}

static void print_page_fault_without_victim(void) {
    printf("Page fault!\n");
}

static void print_page_fault_with_victim(char *victim_lines[PAGE_SIZE], size_t line_count) {
    printf("Page fault! Victim page contents:\n\n");
    for (size_t i = 0; i < line_count; ++i) {
        printf("%s", victim_lines[i]);
        if (victim_lines[i][0] != '\0' && victim_lines[i][strlen(victim_lines[i]) - 1] != '\n') {
            printf("\n");
        }
    }
    printf("\nEnd of victim page contents.\n");
}

static int load_program_page(struct Program *program, size_t page_number, bool announce_fault) {
    char *page_lines[PAGE_SIZE] = {0};
    size_t line_count = 0;
    int frame_index = -1;

    if (page_number >= program->page_count) {
        return -1;
    }

    if (program->page_table[page_number] >= 0) {
        return 0;
    }

    if (read_program_page(program, page_number, page_lines, &line_count) != 0) {
        free_page_lines(page_lines);
        return -1;
    }

    frame_index = frame_store_find_free_frame();
    if (frame_index < 0) {
        if (!announce_fault) {
            free_page_lines(page_lines);
            return -1;
        }

        {
            struct Program *victim_program = NULL;
            size_t victim_page_number = 0;
            char *victim_lines[PAGE_SIZE] = {0};
            size_t victim_line_count = 0;

            frame_index = frame_store_select_lru_victim();
            frame_store_evict_frame(
                frame_index,
                &victim_program,
                &victim_page_number,
                victim_lines,
                &victim_line_count
            );

            if (victim_program) {
                program_invalidate_page(victim_program, victim_page_number);
            }

            print_page_fault_with_victim(victim_lines, victim_line_count);
            free_page_lines(victim_lines);
        }
    } else if (announce_fault) {
        print_page_fault_without_victim();
    }

    frame_store_write_frame(frame_index, program, page_number, page_lines, line_count);
    program->page_table[page_number] = frame_index;
    return 0;
}

struct Program *get_or_create_program(const char *filename, bool *created) {
    char resolved_path[PATH_MAX];
    struct Program *program = NULL;
    size_t line_count = 0;

    if (created) {
        *created = false;
    }

    if (!realpath(filename, resolved_path)) {
        return NULL;
    }

    for (program = program_registry; program; program = program->next) {
        if (strcmp(program->path, resolved_path) == 0) {
            return program;
        }
    }

    line_count = count_program_lines(resolved_path);
    if (line_count == (size_t)(-1)) {
        return NULL;
    }

    program = calloc(1, sizeof(struct Program));
    program->name = strdup(filename);
    program->path = strdup(resolved_path);
    program->line_count = line_count;
    program->page_count = (line_count + PAGE_SIZE - 1) / PAGE_SIZE;

    if (program->page_count > 0) {
        program->page_table = malloc(program->page_count * sizeof(int));
        for (size_t i = 0; i < program->page_count; ++i) {
            program->page_table[i] = -1;
        }
    }

    program->next = program_registry;
    program_registry = program;

    if (created) {
        *created = true;
    }

    return program;
}

void reset_program_registry(void) {
    struct Program *program = program_registry;

    while (program) {
        struct Program *next = program->next;
        free_program(program);
        program = next;
    }

    program_registry = NULL;
}

size_t program_line_count(const struct Program *program) {
    return program->line_count;
}

const char *program_name(const struct Program *program) {
    return program->name;
}

int program_load_initial_pages(struct Program *program, size_t page_limit) {
    size_t pages_to_load = program->page_count < page_limit ? program->page_count : page_limit;

    for (size_t page = 0; page < pages_to_load; ++page) {
        if (load_program_page(program, page, false) != 0) {
            return -1;
        }
    }

    return 0;
}

int program_load_page_on_fault(struct Program *program, size_t page_number) {
    return load_program_page(program, page_number, true);
}

int program_get_frame_for_page(const struct Program *program, size_t page_number) {
    if (page_number >= program->page_count) {
        return -1;
    }
    return program->page_table[page_number];
}

const char *program_get_line(struct Program *program, size_t logical_pc, int *frame_index) {
    size_t page_number = logical_pc / PAGE_SIZE;
    size_t offset = logical_pc % PAGE_SIZE;
    int frame = program_get_frame_for_page(program, page_number);

    if (frame_index) {
        *frame_index = frame;
    }

    if (frame < 0) {
        return NULL;
    }

    return frame_store_get_line(frame, offset);
}

void program_invalidate_page(struct Program *program, size_t page_number) {
    if (page_number < program->page_count) {
        program->page_table[page_number] = -1;
    }
}
