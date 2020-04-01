#ifndef PROCESS_H_
#define PROCESS_H_
#include "../lib/stdint.h"
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define USER_PROG_START_VADDR 0x8048000
#define DIV_ROUND_UP(X, STEP) (X + STEP - 1) / STEP

void start_process(void *filename);
void page_dir_activate(struct pcb_struct *pthread);
void process_activate(struct pcb_struct *pcb);
uint32_t *create_page_dir();
void create_user_vaddr_bitmap(struct pcb_struct *user_prog);
void process_execute(void *filename, char *name);

#endif