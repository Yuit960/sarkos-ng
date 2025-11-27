#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <segmem.h>
#include "segmentation.h"

extern tss_t TSS;
extern seg_desc_t gdt[7];
extern gdt_reg_t gdtr;

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

void init_gdt(void);
void set_selectors(void);
void print_gdt_content(gdt_reg_t gdtr_ptr);
void check_ring(void);

#endif