#include "../include/scheduler.h"
#include "../include/segmentation.h"
#include "../include/paging.h"
#include <segmem.h>
#include <cr.h>

uint8_t kstack_t1[STACK_SIZE] __attribute__((aligned(4096)));
uint8_t kstack_t2[STACK_SIZE] __attribute__((aligned(4096)));
uint8_t ustack_t1[STACK_SIZE] __attribute__((aligned(4096)));
uint8_t ustack_t2[STACK_SIZE] __attribute__((aligned(4096)));

task_t tasks[2];
static int current_task = 0;
volatile uint32_t tmp_esp;

void init_task(int id, void (*func)(), uint8_t *kstack, uint8_t *ustack) {
    task_t *t = &tasks[id];
    t->cr3 = (uint32_t)pgd;
    t->esp0 = (uint32_t)(kstack + STACK_SIZE);

    uint32_t *esp = (uint32_t *)(kstack + STACK_SIZE);

    *(--esp) = gdt_usr_seg_sel(4);              
    *(--esp) = (uint32_t)(ustack + STACK_SIZE); 
    *(--esp) = 0x202;                           
    *(--esp) = gdt_usr_seg_sel(3);              
    *(--esp) = (uint32_t)func;                  
    *(--esp) = 0;                               
    *(--esp) = 0;                               
    *(--esp) = 0;                               
    *(--esp) = 0;                               
    *(--esp) = 0;                               
    *(--esp) = 0;                               
    *(--esp) = 0;                               
    *(--esp) = 0;                               

    t->kstack_top = (uint32_t)esp;
}

void schedule() {
    tasks[current_task].kstack_top = tmp_esp;
    current_task = (current_task + 1) % 2;

    TSS.s0.esp = tasks[current_task].esp0;
    
    set_cr3((void*)tasks[current_task].cr3);

    tmp_esp = tasks[current_task].kstack_top;
}