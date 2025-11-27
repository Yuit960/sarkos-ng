#include "../include/user.h"
#include "../include/utils.h"
#include <debug.h>

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