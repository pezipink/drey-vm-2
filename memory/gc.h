#ifndef _GC_H
#define _GC_H

#include "manager.h"

int gc_clean_sweep();

int gc_clean_full();

int gc_clean_step(int last_offset);

void gc_clean_cycles();


#endif
