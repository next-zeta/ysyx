#ifndef WATCHPOINT_H
#define WATCHPOINT_H

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
    int NO;
    struct watchpoint *next;
    bool success; 
    char expr[100];
    int value;

} WP;

extern WP wp_pool[NR_WP];

void init_wp_pool();
WP* new_wp();
void free_wp(WP *wp);


#endif
