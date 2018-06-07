#ifndef _REFLIST_H
#define _REFLIST_H

#include "..\memory\manager.h"

typedef struct list
{
  memref head;
  memref tail;
} list;

memref rl_init();
memref rl_init_h_t(memref , memref);
memref rl_head(memref);
memref rl_tail(memref);
//memref rl_walk(memref, bool action (memref));

#endif
