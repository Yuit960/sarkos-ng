#ifndef PAGING_H
#define PAGING_H

#include <types.h>

// Définition des adresses partagées (si pas déjà fait ailleurs)
#define SHARED_ADDR_U1  0xCAFE0000
#define SHARED_ADDR_U2  0xBEEF0000

// Prototypes
void init_pagination();
void activate_pagination();

// Fonctions pour récupérer les PGD des tâches (pour le main.c)
uint32_t get_pgd_t1_phys();
uint32_t get_pgd_t2_phys();

#endif