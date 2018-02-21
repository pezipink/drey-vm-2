#ifndef _FIXED_POOL_H
#define _FIXED_POOL_H

typedef struct MemoryPool_Fixed
{
  struct MemoryPool_Fixed** owner;
  unsigned free_offset;
  unsigned element_size;
  unsigned element_count;
  void* data;
} MemoryPool_Fixed;


void fixed_pool_init(MemoryPool_Fixed** owner, unsigned element_size, unsigned initial_element_count);
 void* fixed_pool_get(MemoryPool_Fixed* pool, unsigned offset);
unsigned fixed_pool_alloc(MemoryPool_Fixed* pool);
void fixed_pool_free(MemoryPool_Fixed* pool, unsigned offset);




#endif
