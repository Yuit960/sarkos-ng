/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>

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

seg_desc_t new_gdt[6] = {0};
tss_t TSS;

void userland() {
   debug("hello\n");
   asm volatile ("mov %eax, %cr0");
   
}

void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t* gdt_ptr;
    gdt_ptr = (seg_desc_t*)(gdtr_ptr.addr);
    int i=0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3<<24 | gdt_ptr->base_2<<16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g) {
            end = start + ( (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1) <<12) + 4095;
        } else {
            end = start + (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1);
        }
        debug("%d ", i);
        debug("[0x%x ", start);
        debug("- 0x%x] ", end);
        debug("seg_t: 0x%x ", gdt_ptr->type);
        debug("desc_t: %d ", gdt_ptr->s);
        debug("priv: %d ", gdt_ptr->dpl);
        debug("present: %d ", gdt_ptr->p);
        debug("avl: %d ", gdt_ptr->avl);
        debug("longmode: %d ", gdt_ptr->l);
        debug("default: %d ", gdt_ptr->d);
        debug("gran: %d ", gdt_ptr->g);
        debug("\n");
        gdt_ptr++;
        i++;
    }
}

void tp() {
   //QUESTION 1
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

   // On met les selecteurs sur les bons segments kernel
   set_cs(gdt_krn_seg_sel(1));

   set_ss(gdt_krn_seg_sel(2));
   set_ds(gdt_krn_seg_sel(2));
   set_es(gdt_krn_seg_sel(2));
   set_fs(gdt_krn_seg_sel(2));
   set_gs(gdt_krn_seg_sel(2));

   // On met les selecters sur les bons segments user
   set_ds(gdt_usr_seg_sel(4));
   set_es(gdt_usr_seg_sel(4));
   set_fs(gdt_usr_seg_sel(4));
   set_gs(gdt_usr_seg_sel(4));

   // On config le TSS pour le retour au niveau 0
   TSS.s0.esp = get_ebp();
   TSS.s0.ss  = gdt_krn_seg_sel(2);
   tss_dsc(&new_gdt[5], (offset_t)&TSS);
   set_tr(gdt_krn_seg_sel(5));

   //QUESTION 2
   asm volatile (
   "push %0    \n" // ss
   "push %%ebp \n" // esp
   "pushf      \n" // eflags
   "push %1    \n" // cs
   "push %2    \n" // eip
   // end Q2
   // Q3
   "iret"
   ::
    "i"(gdt_usr_seg_sel(4)),
    "i"(gdt_usr_seg_sel(3)),
    "r"(&userland)
   );

}
