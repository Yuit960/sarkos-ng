#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <intr.h>

void init_interruption(void);
void init_timer(void);
void syscall_isr(void);
void irq0_isr(void);
void __regparm__(1) syscall_handler(int_ctx_t *ctx);

#endif