#include "debug.h"
#include <debug.h>
#include <cr.h>

void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t *gdt_ptr = (seg_desc_t *)(gdtr_ptr.addr);
    int i = 0;
    debug("\n[GDT]\n");
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3 << 24 | gdt_ptr->base_2 << 16 | gdt_ptr->base_1;
        uint32_t end = gdt_ptr->g ? 
            start + ((gdt_ptr->limit_2 << 16 | gdt_ptr->limit_1) << 12) + 4095 :
            start + (gdt_ptr->limit_2 << 16 | gdt_ptr->limit_1);
        
        debug("%d [0x%x - 0x%x] seg_t: 0x%x priv: %d pres: %d gran: %d\n", 
              i, start, end, gdt_ptr->type, gdt_ptr->dpl, gdt_ptr->p, gdt_ptr->g);
        gdt_ptr++; i++;
    }
}

void debug_selector(char *name, uint16_t sel) {
    uint16_t index = sel >> 3;
    uint16_t ti    = (sel >> 2) & 1;
    uint16_t rpl   = sel & 3;

    debug(name);
    debug(" -> Raw: 0x%x | Idx: %d | Tab: ", sel, index);
    debug(ti ? "LDT" : "GDT");
    debug(" | RPL: %d\n", rpl);
}

void debug_infos(gdt_reg_t gdtr_ptr, pde32_t *pgd1, pde32_t *pgd2, void* u1_entry, void* u1_stack, void* u2_entry, void* u2_stack) {
    extern char __kernel_start__[], __kernel_end__[], __user_start__[], __user_end__[];
    
    debug("\n=== BEGIN DEBUG INFOS ===\n");

    print_gdt_content(gdtr_ptr);

    debug("\n[SELECTORS]\n");
    debug_selector("KERNEL Code (Ring0)", gdt_krn_seg_sel(1));
    debug_selector("KERNEL Data (Ring0)", gdt_krn_seg_sel(2));
    debug_selector("USER   Code (Ring3)", gdt_usr_seg_sel(3));
    debug_selector("USER   Data (Ring3)", gdt_usr_seg_sel(4));
    debug_selector("TSS    System",       gdt_krn_seg_sel(5));

    debug("\n[MEMORY]\n");
    debug("Kernel Physical: %p - %p\n", __kernel_start__, __kernel_end__);
    debug("User   Physical: %p - %p\n", __user_start__, __user_end__);

    uint32_t v_addr = 0xCAFE0000;
    uint32_t pgd_idx = pd32_get_idx(v_addr);
    uint32_t ptb_idx = pt32_get_idx(v_addr);
    pte32_t *ptb = (pte32_t *)page_get_addr(pgd1[pgd_idx].addr);
    uint32_t phys = page_get_addr(ptb[ptb_idx].addr);
    
    debug("\n[SHARED MEMORY]\n");
    debug("Task 1 Virtual: 0x%x -> Phys: 0x%x\n", v_addr, phys);
    debug("Task 2 Virtual: 0xBEEF0000 -> Phys: 0x800000\n");

    debug("\n[TASKS]\n");
    debug("Task 1: Entry=%p Stack=%p PGD=0x%x\n", u1_entry, u1_stack, (uint32_t)pgd1);
    debug("Task 2: Entry=%p Stack=%p PGD=0x%x\n", u2_entry, u2_stack, (uint32_t)pgd2);

    debug("\n=== END DEBUG INFOS ===\n");
}