#ifndef _FIXED_POOL_H
#define _FIXED_POOL_H

typedef struct MemoryPool_Fixed
{
  struct MemoryPool_Fixed** owner;
  int free_offset;
  int element_size;
  int element_count;
  void* data;
} MemoryPool_Fixed;


void static_pool_init(int* owner, int dataSize, int initialElementCount);
void* static_pool_get(int staticPool, int offset);
int static_pool_alloc(int staticPool);
void static_pool_free(int staticPool, int offset);




#endif
