#include <stdlib.h>
#include <unistd.h>

#include "pcb.h"
#include "program.h"
#include "shell.h"
#include "shellmemory.h"

int pcb_has_next_instruction(struct PCB *pcb) {
    return pcb->pc < program_line_count(pcb->program);
}

size_t pcb_current_page(const struct PCB *pcb) {
    return pcb->pc / PAGE_SIZE;
}

const char *pcb_current_instruction(struct PCB *pcb, int *frame_index) {
    return program_get_line(pcb->program, pcb->pc, frame_index);
}

struct PCB *create_process(struct Program *program) {
    static pid fresh_pid = 1;
    struct PCB *pcb = calloc(1, sizeof(struct PCB));

    if (!pcb) {
        return NULL;
    }

    pcb->pid = fresh_pid++;
    pcb->name = program_name(program);
    pcb->program = program;
    pcb->duration = program_line_count(program);
    pcb->pc = 0;
    pcb->next = NULL;
    return pcb;
}

struct PCB *create_process_from_FILE(FILE *f) {
    char template[] = "/tmp/mysh-stdin-XXXXXX";
    char linebuf[MAX_USER_INPUT];
    int fd = mkstemp(template);
    FILE *tmp = NULL;
    bool created = false;
    struct Program *program = NULL;

    if (fd < 0) {
        return NULL;
    }

    tmp = fdopen(fd, "w");
    if (!tmp) {
        close(fd);
        unlink(template);
        return NULL;
    }

    while (fgets(linebuf, sizeof(linebuf), f)) {
        fputs(linebuf, tmp);
    }

    fclose(tmp);

    program = get_or_create_program(template, &created);
    if (!program || !created) {
        unlink(template);
        return NULL;
    }

    program->delete_on_reset = true;
    if (program_load_initial_pages(program, 2) != 0) {
        unlink(template);
        return NULL;
    }

    return create_process(program);
}

void advance_pcb(struct PCB *pcb) {
    pcb->pc++;
}

void free_pcb(struct PCB *pcb) {
    free(pcb);
}
