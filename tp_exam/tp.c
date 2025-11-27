/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <pagemem.h>
#include <cr.h>
#include <intr.h>
#include <io.h>

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

void check_ring() {
    uint32_t cs_reg;
    asm volatile("mov %%cs, %0" : "=r"(cs_reg));
    
    if ((cs_reg & 0x3) == 3) {
        debug("Currently RING 3 (CS = 0x%x)\n", cs_reg);
    } else {
        debug("Currently RING 0 (CS = 0x%x)\n", cs_reg);
    }
}



/*SEGMENT FOR DEBUG END*/















#define SHARED_ADDR_U1  0xCAFE0000
#define SHARED_ADDR_U2  0xBEEF0000

#define PRINT_COUNTER_ISR 1
#define CLOCK_ISR 32

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


//SEGMENTATION

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






///PAGINATION

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
        map_page(addr, addr, PG_KRN | PG_RW);
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





//INTERRUPTIONS


void syscall_isr() {
   asm volatile (
      "leave ; pusha        \n"
      "mov %esp, %eax       \n"
      "call syscall_handler \n"
      "popa ; iret"
      );
}

void irq0_isr(void) {
    asm volatile (
        "leave; pusha      \n"
        "call schedule     \n"
        "mov $0x20, %al    \n"
        "out %al, $0x20    \n"
        "popa ; iret       \n"
    );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx){
	uint32_t int_n = ctx->gpr.eax.raw;
	uint32_t param = ctx->gpr.ebx.raw;

	switch (int_n){
	case PRINT_COUNTER_ISR:
		debug("Counter at addr %p, value %d\n", (uint32_t*)param, *((uint32_t*)param));
		break;

	case CLOCK_ISR:
		break;

	default:
		debug("Cannont handle the interruption number %d\n", int_n);
		break;
	}
}

void schedule(){
	debug("Scheduler called.\n");
}

void init_interruption(){
	idt_reg_t idtr;
   	get_idtr(idtr);

	int_desc_t *sys_dsc = &idtr.desc[0x80];
	sys_dsc->offset_1 = (uint16_t)((uint32_t)syscall_isr);
	sys_dsc->offset_2 = (uint16_t)(((uint32_t)syscall_isr)>>16);
	sys_dsc->type = 0xE;
    sys_dsc->p = 1;
    sys_dsc->dpl = 3;

	int_desc_t *clk_dsc = &idtr.desc[32];
	clk_dsc->offset_1 = (uint16_t)((uint32_t)irq0_isr);
	clk_dsc->offset_2 = (uint16_t)(((uint32_t)irq0_isr)>>16);
	clk_dsc->type = 0xE;
    clk_dsc->p = 1;
    clk_dsc->dpl = 0;
}

//CLOCK

void init_timer(){
    uint32_t divisor = 1193180 / 100; // 100 Hz
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
	outb(0x21, 0xFE);
}

void enable_timer(){
	asm volatile("sti");
}


//PRIVILEGE LEVELS

// void switch_ring3(){
// 	asm volatile (
//    "push %0    \n" // ss
//    "push %%ebp \n" // esp
//    "pushf      \n" // eflags
//    "push %1    \n" // cs
//    "push %2    \n" // eip
//    // end Q2
//    // Q3
//    "iret"
//    ::
//     "i"(gdt_usr_seg_sel(4)),
//     "i"(gdt_usr_seg_sel(3)),
//     "r"(&user2)
//    );
// }



//USER FUNCTIONS

__attribute__((section(".user"))) void sys_counter(uint32_t *counter){
	asm volatile (
        "int $0x80"
        :         
        : "a" (1),
		  "b" (counter)  
        : "memory"       
    );
}

__attribute__((section(".user"))) void user1(void){
	uint32_t *counter_addr = (uint32_t *)SHARED_ADDR_U1;
	uint32_t counter = 0;
	while(1){
		*counter_addr = counter++;
		for (volatile int i = 0; i < 100000; i++);
	};
}

__attribute__((section(".user"))) void user2(void){
	uint32_t *counter_addr = (uint32_t *)SHARED_ADDR_U2;
	while(1){
		sys_counter(counter_addr);
	};
}




//MAIN FUNCTION


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

	//Set pagination
	init_pagination();
	activate_pagination();

	//Interruptions
	init_interruption();

	//Timer
	init_timer();
	enable_timer();

	check_ring();





	while(1);

}
