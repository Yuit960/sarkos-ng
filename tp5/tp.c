/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
#include <segmem.h>

seg_desc_t new_gdt[6] = {0};
tss_t TSS;

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

void syscall_isr() {
   asm volatile (
      "leave ; pusha        \n"
      "mov %esp, %eax      \n"
      "call syscall_handler \n"
      "popa ; iret"
      );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx) {
   debug("SYSCALL eax = %p\n", (void *) ctx->gpr.eax.raw);

   //QUESTION 4
   debug("print syscall: %s", (char *)ctx->gpr.esi.raw);
}

void userland() {
   
   debug("Hello\n");
   // uint32_t arg =  0x2023;
   // asm volatile ("int $48"::"a"(arg)); //QUESTION 3

   //QUESTION 5
   asm volatile ("int $48"::"S"(0x3049b9));

   while(1);
}

void start_r3(){
   new_gdt[1].base_1 = 0;
   new_gdt[1].base_2 = 0;
   new_gdt[1].base_3 = 0;
   new_gdt[1].limit_1 = 0xffff;
   new_gdt[1].limit_2 = 0xf;
   new_gdt[1].type = 0xa;
   new_gdt[1].s = 0x1;
   new_gdt[1].dpl = 0x0;
   new_gdt[1].p = 0x1;
   new_gdt[1].avl = 0x1;
   new_gdt[1].l = 0x0;
   new_gdt[1].d = 0x1;
   new_gdt[1].g = 0x1;

   new_gdt[2].base_1 = 0;
   new_gdt[2].base_2 = 0;
   new_gdt[2].base_3 = 0;
   new_gdt[2].limit_1 = 0xffff;
   new_gdt[2].limit_2 = 0xf;
   new_gdt[2].type = 0x2;
   new_gdt[2].s = 0x1;
   new_gdt[2].dpl = 0x0;
   new_gdt[2].p = 0x1;
   new_gdt[2].avl = 0x0;
   new_gdt[2].l = 0x0;
   new_gdt[2].d = 0x1;
   new_gdt[2].g = 0x1;

   new_gdt[3].base_1 = 0;
   new_gdt[3].base_2 = 0;
   new_gdt[3].base_3 = 0;
   new_gdt[3].limit_1 = 0xffff;
   new_gdt[3].limit_2 = 0xf;
   new_gdt[3].type = 0xa;
   new_gdt[3].s = 0x1;
   new_gdt[3].dpl = 0x3;
   new_gdt[3].p = 0x1;
   new_gdt[3].avl = 0x1;
   new_gdt[3].l = 0x0;
   new_gdt[3].d = 0x1;
   new_gdt[3].g = 0x1;

   new_gdt[4].base_1 = 0;
   new_gdt[4].base_2 = 0;
   new_gdt[4].base_3 = 0;
   new_gdt[4].limit_1 = 0xffff;
   new_gdt[4].limit_2 = 0xf;
   new_gdt[4].type = 0x2;
   new_gdt[4].s = 0x1;
   new_gdt[4].dpl = 0x3;
   new_gdt[4].p = 0x1;
   new_gdt[4].avl = 0x0;
   new_gdt[4].l = 0x0;
   new_gdt[4].d = 0x1;
   new_gdt[4].g = 0x1;

   gdt_reg_t new_gdt_reg;
   new_gdt_reg.addr = (long unsigned int)new_gdt;
   new_gdt_reg.limit = sizeof(new_gdt) - 1;
   set_gdtr(new_gdt_reg);

   set_cs(gdt_krn_seg_sel(1));

   set_ss(gdt_krn_seg_sel(2));
   set_ds(gdt_krn_seg_sel(2));
   set_es(gdt_krn_seg_sel(2));
   set_fs(gdt_krn_seg_sel(2));
   set_gs(gdt_krn_seg_sel(2));

   set_ds(gdt_usr_seg_sel(4));
   set_es(gdt_usr_seg_sel(4));
   set_fs(gdt_usr_seg_sel(4));
   set_gs(gdt_usr_seg_sel(4));

   TSS.s0.esp = get_ebp();
   TSS.s0.ss  = gdt_krn_seg_sel(2);
   tss_dsc(&new_gdt[5], (offset_t)&TSS);
   set_tr(gdt_krn_seg_sel(5));
}

void update_idt(){
   idt_reg_t idtr_reg;
	get_idtr(idtr_reg);
   idtr_reg.desc[48].offset_1 = ((uint32_t)&syscall_isr) & 0xffff;
	idtr_reg.desc[48].offset_2 = ((uint32_t)&syscall_isr >> 16) & 0xffff;
   idtr_reg.desc[48].dpl = 3; //QUESTION 3*
}

void tp() {
   start_r3(); //QUESTION 1
   update_idt(); //QUESTION 2

   asm volatile (
   "push %0    \n" // ss
   "push %%ebp \n" // esp
   "pushf      \n" // eflags
   "push %1    \n" // cs
   "push %2    \n" // eip
   "iret"
   ::
    "i"(gdt_usr_seg_sel(4)),
    "i"(gdt_usr_seg_sel(3)),
    "r"(&userland)
   );
}
