#ifndef DEBUG_H
#define DEBUG_H

#include <segmem.h>
#include <pagemem.h>

void print_gdt_content(gdt_reg_t gdtr_ptr);

void check_ring();

void debug_selector(char *name, uint16_t sel);

void debug_infos(gdt_reg_t gdtr_ptr, pde32_t *pgd1, pde32_t *pgd2, void* u1_entry, void* u1_stack, void* u2_entry, void* u2_stack);

#endif