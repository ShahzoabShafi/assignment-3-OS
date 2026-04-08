#pragma once

#include <stddef.h>
#include <stdio.h>

typedef size_t pid;

struct Program;

struct PCB {
    pid pid;
    const char *name;
    struct Program *program;
    size_t duration;
    size_t pc;
    struct PCB *next;
};

int pcb_has_next_instruction(struct PCB *pcb);
size_t pcb_current_page(const struct PCB *pcb);
const char *pcb_current_instruction(struct PCB *pcb, int *frame_index);
struct PCB *create_process(struct Program *program);
struct PCB *create_process_from_FILE(FILE *f);
void advance_pcb(struct PCB *pcb);
void free_pcb(struct PCB *pcb);
