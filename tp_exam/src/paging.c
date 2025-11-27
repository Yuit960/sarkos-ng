#include "../include/paging.h"
#include <segmem.h>
#include <pagemem.h>
#include <debug.h>
#include <types.h>
#include <cr.h>
#include <string.h> // Pour memset

// 1. Création de deux répertoires de pages distincts (alignés 4K)
// On utilise 'static' pour qu'ils soient privés à ce fichier, on y accède via les getters
static pde32_t pgd_t1[1024] __attribute__((aligned(4096)));
static pde32_t pgd_t2[1024] __attribute__((aligned(4096)));

// 2. Allocateur simple pour les Page Tables (PTB)
// On commence à allouer les PTB après la zone réservée (ex: 0x610000)
// Cela évite que T1 et T2 écrasent les PTB de l'autre.
static uint32_t next_ptb_phys_addr = 0x610000;

// Fonction interne modifiée : prend le PGD cible en paramètre
void map_page_in_pgd(pde32_t *target_pgd, uint32_t phys_addr, uint32_t virt_addr, uint32_t flags) {
    uint32_t pgd_idx = pd32_get_idx(virt_addr);
    uint32_t ptb_idx = pt32_get_idx(virt_addr);

    pde32_t *pde = &target_pgd[pgd_idx];
    pte32_t *ptb;

    if (!pde->p) {
        // --- ALLOCATION DYNAMIQUE ---
        // On prend une nouvelle page physique libre pour la PTB
        uint32_t new_ptb_phys = next_ptb_phys_addr;
        next_ptb_phys_addr += 4096; // On incrémente pour la prochaine allocation

        ptb = (pte32_t *)new_ptb_phys;
        memset(ptb, 0, PAGE_SIZE);
        
        // On lie le PDE du PGD cible vers cette nouvelle PTB
        pg_set_entry(pde, PG_USR | PG_RW, page_get_nr(new_ptb_phys));
    } else {
        // La PTB existe déjà pour ce PGD
        ptb = (pte32_t *)(page_get_addr(pde->addr));
    }

    // On écrit l'entrée dans la PTB
    pg_set_entry(&ptb[ptb_idx], flags, page_get_nr(phys_addr));
}

void init_pagination(){
    // Nettoyage des deux PGDs
    memset(pgd_t1, 0, sizeof(pgd_t1));
    memset(pgd_t2, 0, sizeof(pgd_t2));

    // 1. Identity Mapping (Noyau + Code User) pour les DEUX tâches
    // De 0 à 8MB (couvre le kernel et tes sections user .text)
    for (uint32_t addr = 0; addr < 0x800000; addr += 0x1000) {
        // On map la même chose dans les deux contextes
        map_page_in_pgd(pgd_t1, addr, addr, PG_KRN | PG_USR | PG_RW);
        map_page_in_pgd(pgd_t2, addr, addr, PG_KRN | PG_USR | PG_RW);
    }

    // 2. Mapping de la mémoire partagée (LA partie spécifique)
    uint32_t shared_frame = 0x800000; // La frame physique réelle

    // Tâche 1 : voit la frame à l'adresse virtuelle U1
    map_page_in_pgd(pgd_t1, shared_frame, SHARED_ADDR_U1, PG_USR | PG_RW);

    // Tâche 2 : voit la frame à l'adresse virtuelle U2
    map_page_in_pgd(pgd_t2, shared_frame, SHARED_ADDR_U2, PG_USR | PG_RW);
    
    // On charge le PGD de la tâche 1 par défaut pour le démarrage
    set_cr3(pgd_t1);
}

void activate_pagination(){
    cr0_reg_t cr0 = {.raw = get_cr0()};
    set_cr0(cr0.raw | CR0_PG);
}

// Getters pour main.c
uint32_t get_pgd_t1_phys() { return (uint32_t)pgd_t1; }
uint32_t get_pgd_t2_phys() { return (uint32_t)pgd_t2; }