/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include "include/segmentation.h"
#include "include/paging.h"
#include "include/interrupts.h"
#include "include/scheduler.h"
#include "include/user.h"

void tp() {
    init_gdt();
    gdtr.addr = (long unsigned int)gdt;
    gdtr.limit = sizeof(gdt) - 1;
    set_gdtr(gdtr);

    TSS.s0.esp = get_ebp();
    TSS.s0.ss  = gdt_krn_seg_sel(2);
    tss_dsc(&gdt[5], (offset_t)&TSS);

    set_selectors();

    init_pagination();
    activate_pagination();

    init_interruption();

    init_timer();

    print_gdt_content(gdtr);
    
    init_task(0, user1, kstack_t1, ustack_t1);
    init_task(1, user2, kstack_t2, ustack_t2);

    TSS.s0.esp = tasks[0].esp0;
    asm volatile (
        "mov %0, %%esp \n"
        "popa          \n"
        "iret          \n"
        : : "r" (tasks[0].kstack_top)
    );

    while(1);
}