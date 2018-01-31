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


void fixed_pool_init(MemoryPool_Fixed** owner, int element_size, int initial_element_count);
void* fixed_pool_get(MemoryPool_Fixed* pool, int offset);
int fixed_pool_alloc(MemoryPool_Fixed* pool);
void fixed_pool_free(MemoryPool_Fixed* pool, int offset);




#endif
