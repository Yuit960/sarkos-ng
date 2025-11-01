/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>

void tp() {
	//QUESTION 1
	cr3_reg_t cr3 = {.raw = get_cr3()};
	debug("CR3: 0x%x\n", cr3.raw);

	//QUESTION 2
	pde32_t *pgd = (pde32_t *)0x600000;
	set_cr3(pgd);

	//QUESTION 3
	// cr0_reg_t cr0 = {.raw = get_cr0()};
	// set_cr0(cr0.raw|CR0_PG);

	//QUESTION 4
	pte32_t *ptb = (pte32_t *)0x601000;

	//QUESTION 5
	for (int i = 0; i<1024; i++){
		pg_set_entry(&ptb[i], PG_KRN|PG_RW, i);
	}
	memset(pgd, 0, PAGE_SIZE);
	pg_set_entry(&pgd[0], PG_KRN|PG_RW, page_get_nr(ptb));
	// cr0_reg_t cr0 = {.raw = get_cr0()};
	// set_cr0(cr0.raw|CR0_PG);

	//QUESTION 6
	pte32_t *ptb2 = (pte32_t *)0x602000;
	for (int i = 0; i<1024; i++){
		pg_set_entry(&ptb2[i], PG_KRN|PG_RW, i+1024);
	}
	memset(&pgd[1], 0, PAGE_SIZE);
	pg_set_entry(&pgd[1], PG_KRN|PG_RW, page_get_nr(ptb2));
	// cr0_reg_t cr0 = {.raw = get_cr0()};
	// set_cr0(cr0.raw|CR0_PG);
	debug("PTB entry 0: 0x%x\n", ptb[0].raw);

	//QUESTION 7
	pte32_t  *ptb3    = (pte32_t*)0x603000;
	uint32_t *target  = (uint32_t*)0xc0000000;
	int      pgd_idx = pd32_get_idx(target);
	int      ptb_idx = pt32_get_idx(target);
	memset((void*)ptb3, 0, PAGE_SIZE);
	pg_set_entry(&ptb3[ptb_idx], PG_KRN|PG_RW, page_get_nr(pgd));
	pg_set_entry(&pgd[pgd_idx], PG_KRN|PG_RW, page_get_nr(ptb3));
	debug("PGD[0] = 0x%x | target = 0x%x\n", (unsigned int) pgd[0].raw, (unsigned int) *target);

	// uint32_t cr0 = get_cr0();
	// set_cr0(cr0|CR0_PG);

	//QUESTION 8
	char *v1 = (char*)0x700000;
	char *v2 = (char*)0x7ff000;
	ptb_idx = pt32_get_idx(v1);
	pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);
	ptb_idx = pt32_get_idx(v2);
	pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);
	cr0_reg_t cr0 = {.raw = get_cr0()};
	set_cr0(cr0.raw|CR0_PG);
	debug("%p = %s | %p = %s\n", v1, v1, v2, v2);

	//QUESTION 9
	// *target = 0;
	// invalidate(target);
}
