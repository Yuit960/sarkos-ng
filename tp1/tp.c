/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>

void userland() {
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
	//QUESTION 2
   debug("\nCurrent GDT:\n");
   gdt_reg_t gdt;
   get_gdtr(gdt);
   print_gdt_content(gdt);


   debug("\nSegments selectors:\n");
   debug("CS index: %d\n", get_cs() >> 3);
   debug("DS index: %d\n", get_ds() >> 3);
   debug("SS index: %d\n", get_ss() >> 3);
   debug("ES index: %d\n", get_es() >> 3);
   debug("FS index: %d\n", get_fs() >> 3);
   debug("GS ndex: %d\n", get_gs() >> 3);

   //QUESTION 5
   seg_desc_t new_gdt[3];
   new_gdt[0] = (seg_desc_t){0};

   new_gdt[1].base_1 = 0;
   new_gdt[1].base_2 = 0;
   new_gdt[1].base_3 = 0;
   new_gdt[1].limit_1 = 0xffff;
   new_gdt[1].limit_2 = 0xf;
   new_gdt[1].type = 0xb;
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
   new_gdt[2].type = 0x3;
   new_gdt[2].s = 0x1;
   new_gdt[2].dpl = 0x0;
   new_gdt[2].p = 0x1;
   new_gdt[2].avl = 0x0;
   new_gdt[2].l = 0x0;
   new_gdt[2].d = 0x1;
   new_gdt[2].g = 0x1;

   //QUESTION 6
   gdt_reg_t new_gdt_reg;
   new_gdt_reg.addr = (long unsigned int)new_gdt;
   new_gdt_reg.limit = sizeof(new_gdt) - 1;

   set_gdtr(new_gdt_reg);
   set_ds(gdt_krn_seg_sel(2));
   set_cs(gdt_krn_seg_sel(1));

   //QUESTION 7
   debug("\nNew GDT:\n");
   get_gdtr(gdt);
   print_gdt_content(gdt);

   //QUESTION 8
   set_ds(gdt_krn_seg_sel(1));

}
