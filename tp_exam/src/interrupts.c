#include "../include/interrupts.h"
#include "../include/scheduler.h"
#include "../include/utils.h"
#include <io.h>
#include <debug.h>

void syscall_isr() {
   asm volatile (
      "leave ; pusha        \n"
      "mov %esp, %eax       \n"
      "call syscall_handler \n"
      "popa ; iret"
      );
}

void irq0_isr(void) {
    asm volatile (
        "leave              \n"
        "pusha              \n"
        "mov 36(%esp), %eax \n"
        "and $0x3, %eax     \n"
        "cmp $0x3, %eax     \n"
        "jne isr_end        \n"
        "movl %esp, tmp_esp \n"
        "call schedule      \n"
        "movl tmp_esp, %esp \n"
        "isr_end:           \n"
        "mov $0x20, %al     \n"
        "out %al, $0x20     \n"
        "popa               \n"
        "iret               \n"
    );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx){
    uint32_t int_n = ctx->gpr.eax.raw;
    uint32_t param = ctx->gpr.ebx.raw;

    switch (int_n){
    case PRINT_COUNTER_ISR:
        debug("Counter at addr %p, value %d\n", (uint32_t*)param, *((uint32_t*)param));
        break;

    default:
        debug("Cannont handle the interruption number %d\n", int_n);
        break;
    }
}

void init_interruption(){
    idt_reg_t idtr;
    get_idtr(idtr);

    int_desc_t *sys_dsc = &idtr.desc[0x80];
    sys_dsc->offset_1 = (uint16_t)((uint32_t)syscall_isr);
    sys_dsc->offset_2 = (uint16_t)(((uint32_t)syscall_isr)>>16);
    sys_dsc->type = 0xE;
    sys_dsc->p = 1;
    sys_dsc->dpl = 3;

    int_desc_t *clk_dsc = &idtr.desc[32];
    clk_dsc->offset_1 = (uint16_t)((uint32_t)irq0_isr);
    clk_dsc->offset_2 = (uint16_t)(((uint32_t)irq0_isr)>>16);
    clk_dsc->type = 0xE;
    clk_dsc->p = 1;
    clk_dsc->dpl = 0;
}

void init_timer(){
    uint32_t divisor = 1193180 / 100; 
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    outb(0x21, 0xFE);
}