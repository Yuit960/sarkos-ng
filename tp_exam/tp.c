/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <pagemem.h>
#include <cr.h>

/*SEGMENT FOR DEBUG START*/


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



/*SEGMENT FOR DEBUT END*/















#define SHARED_ADDR_U1  0xCAFE0000
#define SHARED_ADDR_U2  0xBEEF0000

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

static tss_t TSS;
static seg_desc_t gdt[7];
static gdt_reg_t gdtr;
static pde32_t *pgd = (pde32_t *)0x600000;

void init_gdt(){
	//RING 0 Code
	gdt[1].base_1 = 0;
   	gdt[1].base_2 = 0;
	gdt[1].base_3 = 0;
	gdt[1].limit_1 = 0xffff;
	gdt[1].limit_2 = 0xf;
	gdt[1].type = 0xa;
	gdt[1].s = 0x1;
	gdt[1].dpl = 0x0;
	gdt[1].p = 0x1;
	gdt[1].avl = 0x1;
	gdt[1].l = 0x0;
	gdt[1].d = 0x1;
	gdt[1].g = 0x1;

	//RING 0 Data
	gdt[2].base_1 = 0;
	gdt[2].base_2 = 0;
	gdt[2].base_3 = 0;
	gdt[2].limit_1 = 0xffff;
	gdt[2].limit_2 = 0xf;
	gdt[2].type = 0x2;
	gdt[2].s = 0x1;
	gdt[2].dpl = 0x0;
	gdt[2].p = 0x1;
	gdt[2].avl = 0x0;
	gdt[2].l = 0x0;
	gdt[2].d = 0x1;
	gdt[2].g = 0x1;

	//RING 3 Code
	gdt[3].base_1 = 0;
	gdt[3].base_2 = 0;
	gdt[3].base_3 = 0;
	gdt[3].limit_1 = 0xffff;
	gdt[3].limit_2 = 0xf;
	gdt[3].type = 0xa;
	gdt[3].s = 0x1;
	gdt[3].dpl = 0x3;
	gdt[3].p = 0x1;
	gdt[3].avl = 0x1;
	gdt[3].l = 0x0;
	gdt[3].d = 0x1;
	gdt[3].g = 0x1;

	//RING 3 Data
	gdt[4].base_1 = 0;
	gdt[4].base_2 = 0;
	gdt[4].base_3 = 0;
	gdt[4].limit_1 = 0xffff;
	gdt[4].limit_2 = 0xf;
	gdt[4].type = 0x2;
	gdt[4].s = 0x1;
	gdt[4].dpl = 0x3;
	gdt[4].p = 0x1;
	gdt[4].avl = 0x0;
	gdt[4].l = 0x0;
	gdt[4].d = 0x1;
	gdt[4].g = 0x1;
}

void init_pagination(){
	pte32_t *ptb = (pte32_t *)0x601000;
	for (int i = 0; i<1024; i++){
		pg_set_entry(&ptb[i], PG_KRN|PG_RW, i);
	}
	memset(pgd, 0, PAGE_SIZE);
	pg_set_entry(&pgd[0], PG_KRN|PG_RW, page_get_nr(ptb));
}

void set_selectors(){

	//RING 0 selectors
	set_cs(gdt_krn_seg_sel(1));

	set_ss(gdt_krn_seg_sel(2));
	set_ds(gdt_krn_seg_sel(2));
	set_es(gdt_krn_seg_sel(2));
	set_fs(gdt_krn_seg_sel(2));
	set_gs(gdt_krn_seg_sel(2));

	//RING 3 selectors
	set_ds(gdt_usr_seg_sel(4));
	set_es(gdt_usr_seg_sel(4));
	set_fs(gdt_usr_seg_sel(4));
	set_gs(gdt_usr_seg_sel(4));
	
	//TSS selector
	set_tr(gdt_krn_seg_sel(5));
}

__attribute__((section(".user"))) void user1(void){
	volatile uint32_t *counter_addr = (volatile uint32_t *)SHARED_ADDR_U1;
	uint32_t counter = 0;
	while(1){
		*counter_addr = counter++;
		for (volatile int i = 0; i < 100000; i++);
	};
}

__attribute__((section(".user"))) void user2(void){
	while(1);
}

void tp() {
	//Create GDT
	init_gdt();
	gdtr.addr = (long unsigned int)gdt;
   	gdtr.limit = sizeof(gdt) - 1;
	set_gdtr(gdtr);

	//Init TSS
	TSS.s0.esp = get_ebp();
	TSS.s0.ss  = gdt_krn_seg_sel(2);
	tss_dsc(&gdt[5], (offset_t)&TSS);

	//Selectors
	set_selectors();

	print_gdt_content(gdtr);

	//Set pagination
	init_pagination();
	//activate pagination
	
}
