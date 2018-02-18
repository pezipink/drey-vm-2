#ifndef _REFSTACK_H
#define _REFSTACK_H

#include "..\memory\manager.h"
typedef struct refstack
{
  MemoryPool_Fixed* pool;
  int head_offset;
} refstack;


memref stack_init(int initialSize);
memref stack_peek(memref stack);
memref stack_pop(memref stack);
void stack_push(memref stack, memref ref);
  
#endif
