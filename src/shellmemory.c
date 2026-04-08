#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shellmemory.h"

struct variable_slot {
    char *var;
    char *value;
};

struct frame_slot {
    bool occupied;
    char *lines[PAGE_SIZE];
    size_t line_count;
    unsigned long long last_used;
    struct Program *program;
    size_t page_number;
};

static struct variable_slot variable_store[VAR_STORE_SIZE];
static struct frame_slot frame_store[FRAME_STORE_SIZE / PAGE_SIZE];
static unsigned long long lru_clock = 1;

static void clear_frame_slot(struct frame_slot *frame) {
    for (size_t i = 0; i < PAGE_SIZE; ++i) {
        free(frame->lines[i]);
        frame->lines[i] = NULL;
    }
    frame->occupied = false;
    frame->line_count = 0;
    frame->last_used = 0;
    frame->program = NULL;
    frame->page_number = 0;
}

void mem_init(void) {
    for (size_t i = 0; i < VAR_STORE_SIZE; ++i) {
        variable_store[i].var = NULL;
        variable_store[i].value = NULL;
    }

    for (size_t i = 0; i < FRAME_STORE_SIZE / PAGE_SIZE; ++i) {
        frame_store[i].occupied = false;
        frame_store[i].line_count = 0;
        frame_store[i].last_used = 0;
        frame_store[i].program = NULL;
        frame_store[i].page_number = 0;
        for (size_t j = 0; j < PAGE_SIZE; ++j) {
            frame_store[i].lines[j] = NULL;
        }
    }

    lru_clock = 1;
}

void mem_reset_frame_store(void) {
    for (size_t i = 0; i < FRAME_STORE_SIZE / PAGE_SIZE; ++i) {
        clear_frame_slot(&frame_store[i]);
    }
    lru_clock = 1;
}

void mem_set_value(char *var_in, char *value_in) {
    for (size_t i = 0; i < VAR_STORE_SIZE; ++i) {
        if (variable_store[i].var && strcmp(variable_store[i].var, var_in) == 0) {
            free(variable_store[i].value);
            variable_store[i].value = strdup(value_in);
            return;
        }
    }

    for (size_t i = 0; i < VAR_STORE_SIZE; ++i) {
        if (!variable_store[i].var) {
            variable_store[i].var = strdup(var_in);
            variable_store[i].value = strdup(value_in);
            return;
        }
    }
}

char *mem_get_value(char *var_in) {
    for (size_t i = 0; i < VAR_STORE_SIZE; ++i) {
        if (variable_store[i].var && strcmp(variable_store[i].var, var_in) == 0) {
            return strdup(variable_store[i].value);
        }
    }
    return NULL;
}

size_t mem_get_frame_store_size(void) {
    return FRAME_STORE_SIZE;
}

size_t mem_get_var_store_size(void) {
    return VAR_STORE_SIZE;
}

size_t mem_get_frame_count(void) {
    return FRAME_STORE_SIZE / PAGE_SIZE;
}

int frame_store_find_free_frame(void) {
    for (size_t i = 0; i < mem_get_frame_count(); ++i) {
        if (!frame_store[i].occupied) {
            return (int)i;
        }
    }
    return -1;
}

int frame_store_select_lru_victim(void) {
    int victim = -1;
    unsigned long long oldest = 0;

    for (size_t i = 0; i < mem_get_frame_count(); ++i) {
        if (!frame_store[i].occupied) {
            continue;
        }
        if (victim < 0 || frame_store[i].last_used < oldest) {
            victim = (int)i;
            oldest = frame_store[i].last_used;
        }
    }

    return victim;
}

void frame_store_touch(int frame_index) {
    assert(frame_index >= 0);
    assert((size_t)frame_index < mem_get_frame_count());
    assert(frame_store[frame_index].occupied);
    frame_store[frame_index].last_used = lru_clock++;
}

const char *frame_store_get_line(int frame_index, size_t offset) {
    assert(frame_index >= 0);
    assert((size_t)frame_index < mem_get_frame_count());
    assert(frame_store[frame_index].occupied);
    assert(offset < frame_store[frame_index].line_count);
    return frame_store[frame_index].lines[offset];
}

void frame_store_evict_frame(
    int frame_index,
    struct Program **victim_program,
    size_t *victim_page_number,
    char *victim_lines[PAGE_SIZE],
    size_t *victim_line_count
) {
    struct frame_slot *frame = &frame_store[frame_index];

    assert(frame_index >= 0);
    assert((size_t)frame_index < mem_get_frame_count());
    assert(frame->occupied);

    if (victim_program) {
        *victim_program = frame->program;
    }
    if (victim_page_number) {
        *victim_page_number = frame->page_number;
    }
    if (victim_line_count) {
        *victim_line_count = frame->line_count;
    }

    for (size_t i = 0; i < PAGE_SIZE; ++i) {
        if (victim_lines) {
            victim_lines[i] = frame->lines[i];
        } else {
            free(frame->lines[i]);
        }
        frame->lines[i] = NULL;
    }

    frame->occupied = false;
    frame->line_count = 0;
    frame->last_used = 0;
    frame->program = NULL;
    frame->page_number = 0;
}

void frame_store_write_frame(
    int frame_index,
    struct Program *program,
    size_t page_number,
    char *page_lines[PAGE_SIZE],
    size_t line_count
) {
    struct frame_slot *frame = &frame_store[frame_index];

    assert(frame_index >= 0);
    assert((size_t)frame_index < mem_get_frame_count());
    assert(line_count <= PAGE_SIZE);

    clear_frame_slot(frame);
    frame->occupied = true;
    frame->line_count = line_count;
    frame->program = program;
    frame->page_number = page_number;
    frame->last_used = lru_clock++;

    for (size_t i = 0; i < PAGE_SIZE; ++i) {
        if (i < line_count) {
            frame->lines[i] = page_lines[i];
            page_lines[i] = NULL;
        } else {
            frame->lines[i] = NULL;
        }
    }
}
