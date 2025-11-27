#ifndef PAGING_H
#define PAGING_H

#include <pagemem.h>
#include "utils.h"

extern pde32_t *pgd;

void init_pagination(void);
void activate_pagination(void);
void map_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

#endif