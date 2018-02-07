#ifndef _REFSTACK_H
#define _REFSTACK_H
#include "..\memory\fixed_pool.h"

typedef struct refstack
{
  // stack has its own static pool of memory
  // since we only ever push to the top,
  // we know it wont "fragment" at the bottom,
  // and it will be auto resized 
  MemoryPool_Fixed* pool;
  int head_offset;
} refstack;


#endif
