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

#define STACK_SIZE 4096

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


typedef struct {
    uint32_t kstack_top;
    uint32_t cr3;
    uint32_t esp0;
} task_t;

static tss_t TSS;
static seg_desc_t gdt[7];
static gdt_reg_t gdtr;

static pde32_t pgd_t1[1024] __attribute__((aligned(4096)));
static pde32_t pgd_t2[1024] __attribute__((aligned(4096)));
static uint32_t next_ptb_phys_addr = 0x610000;

uint8_t kstack_t1[STACK_SIZE] __attribute__((aligned(4096)));
uint8_t kstack_t2[STACK_SIZE] __attribute__((aligned(4096)));
uint8_t ustack_t1[STACK_SIZE] __attribute__((aligned(4096)));
uint8_t ustack_t2[STACK_SIZE] __attribute__((aligned(4096)));

static task_t tasks[2];
static int current_task = 0;
volatile uint32_t tmp_esp;

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

void map_page(pde32_t *target_pgd, uint32_t phys_addr, uint32_t virt_addr, uint32_t flags) {
    uint32_t pgd_idx = pd32_get_idx(virt_addr);
    uint32_t ptb_idx = pt32_get_idx(virt_addr);

    pde32_t *pde = &target_pgd[pgd_idx];
    pte32_t *ptb;

    if (!pde->p) {
        uint32_t new_ptb_phys = next_ptb_phys_addr;
        next_ptb_phys_addr += 4096;

        ptb = (pte32_t *)new_ptb_phys;
        memset(ptb, 0, PAGE_SIZE);
        
        pg_set_entry(pde, PG_USR | PG_RW, page_get_nr(new_ptb_phys));
    } else {
        ptb = (pte32_t *)(page_get_addr(pde->addr));
    }

    pg_set_entry(&ptb[ptb_idx], flags, page_get_nr(phys_addr));
}

void init_pagination(){
    memset(pgd_t1, 0, sizeof(pgd_t1));
    memset(pgd_t2, 0, sizeof(pgd_t2));
    
    for (uint32_t addr = 0; addr < 0x800000; addr += 0x1000) {
        map_page(pgd_t1, addr, addr, PG_KRN | PG_USR | PG_RW);
        map_page(pgd_t2, addr, addr, PG_KRN | PG_USR | PG_RW);
    }

    uint32_t shared_frame = 0x800000;

    map_page(pgd_t1, shared_frame, SHARED_ADDR_U1, PG_USR | PG_RW);
    map_page(pgd_t2, shared_frame, SHARED_ADDR_U2, PG_USR | PG_RW);
    
    set_cr3(pgd_t1);
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
        "leave ; pusha      \n"
        "movl %esp, tmp_esp \n"
        "call schedule      \n"
        "movl tmp_esp, %esp \n"
        "mov $0x20, %al     \n"
        "out %al, $0x20     \n"
        "popa ; iret        \n"
    );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx){
    uint32_t int_n = ctx->gpr.eax.raw;
    uint32_t param = ctx->gpr.ebx.raw;

    switch (int_n){
    case PRINT_COUNTER_ISR:
        debug("Counter at addr %p, value %d\n", (uint32_t*)param, *((uint32_t*)param));
        break;

    default:
        debug("Cannont handle the interruption number %d\n", int_n);
        break;
    }
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



//TASKS

void init_task(int id, void (*func)(), uint8_t *kstack, uint8_t *ustack, uint32_t pd_phys) {
    task_t *t = &tasks[id];
    t->cr3 = pd_phys;
    t->esp0 = (uint32_t)(kstack + STACK_SIZE);

    uint32_t *esp = (uint32_t *)(kstack + STACK_SIZE);

    *(--esp) = gdt_usr_seg_sel(4);              // SS
    *(--esp) = (uint32_t)(ustack + STACK_SIZE); // ESP
    *(--esp) = 0x202;                           // EFLAGS
    *(--esp) = gdt_usr_seg_sel(3);              // CS
    *(--esp) = (uint32_t)func;                  // EIP
    *(--esp) = 0;                               // EAX
    *(--esp) = 0;                               // ECX
    *(--esp) = 0;                               // EDX
    *(--esp) = 0;                               // EBX
    *(--esp) = 0;                               // ESP
    *(--esp) = 0;                               // EBP
    *(--esp) = 0;                               // ESI
    *(--esp) = 0;                               // EDI

    t->kstack_top = (uint32_t)esp;
}


void schedule() {
    tasks[current_task].kstack_top = tmp_esp;
    current_task = (current_task + 1) % 2;

    TSS.s0.esp = tasks[current_task].esp0;
    
    set_cr3((void*)tasks[current_task].cr3);

    tmp_esp = tasks[current_task].kstack_top;
}


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
    debug("user1\n");
    uint32_t *counter_addr = (uint32_t *)SHARED_ADDR_U1;
    uint32_t counter = 0;
    while(1){
        *counter_addr = counter++;
        for (volatile int i = 0; i < 100000; i++);
    };
}

__attribute__((section(".user"))) void user2(void){
    debug("user2\n");
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

    print_gdt_content(gdtr);
    
    //Tasks
    init_task(0, user1, kstack_t1, ustack_t1, (uint32_t)pgd_t1);
    init_task(1, user2, kstack_t2, ustack_t2, (uint32_t)pgd_t2);

    //Launch first task
    current_task = 0;
    TSS.s0.esp = tasks[0].esp0;
    asm volatile (
        "mov %0, %%esp \n"
        "popa          \n"
        "iret          \n"
        : : "r" (tasks[0].kstack_top)
    );

    while(1);

}