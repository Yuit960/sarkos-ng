#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "utils.h"
#include <debug.h> 

typedef struct {
    uint32_t kstack_top;
    uint32_t cr3;
    uint32_t esp0;
} task_t;

extern volatile uint32_t tmp_esp;
extern task_t tasks[2];
extern uint8_t kstack_t1[STACK_SIZE];
extern uint8_t ustack_t1[STACK_SIZE];
extern uint8_t kstack_t2[STACK_SIZE];
extern uint8_t ustack_t2[STACK_SIZE];

void init_task(int id, void (*func)(), uint8_t *kstack, uint8_t *ustack);
void schedule(void);

#endif