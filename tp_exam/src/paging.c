#include "../include/paging.h"
#include <segmem.h>
#include <cr.h>

pde32_t *pgd = (pde32_t *)0x600000;

void map_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags) {
    uint32_t pgd_idx = pd32_get_idx(virt_addr);
    uint32_t ptb_idx = pt32_get_idx(virt_addr);

    pde32_t *pde = &pgd[pgd_idx];
    pte32_t *ptb;

    if (!pde->p) {
        uint32_t new_ptb_phys = 0x601000 + (pgd_idx * 0x1000);
        ptb = (pte32_t *)new_ptb_phys;
        memset(ptb, 0, PAGE_SIZE);
        pg_set_entry(pde, PG_USR | PG_RW, page_get_nr(ptb));
    } else {
        ptb = (pte32_t *)(page_get_addr(pde->addr));
    }

    pg_set_entry(&ptb[ptb_idx], flags, page_get_nr(phys_addr));
}

void init_pagination(){
    set_cr3(pgd);
    memset(pgd, 0, PAGE_SIZE);
    
    for (uint32_t addr = 0; addr < 0x400000; addr += 0x1000) {
        map_page(addr, addr, PG_KRN | PG_USR | PG_RW);
    }

    for (uint32_t addr = 0x400000; addr < 0x800000; addr += 0x1000) {
        map_page(addr, addr, PG_USR | PG_RW);
    }

    uint32_t shared_frame = 0x800000;
    map_page(shared_frame, SHARED_ADDR_U1, PG_USR | PG_RW);
    map_page(shared_frame, SHARED_ADDR_U2, PG_USR | PG_RW);
}

void activate_pagination(){
    cr0_reg_t cr0 = {.raw = get_cr0()};
    set_cr0(cr0.raw|CR0_PG);
}