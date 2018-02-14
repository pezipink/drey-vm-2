#ifndef _DYNAMIC_POOL_H
#define _DYNAMIC_POOL_H

typedef struct MemoryPool_Dynamic
{
  struct MemoryPool_Dynamic** owner;
  unsigned total_size;
  unsigned free_offset;
  void* data;
} MemoryPool_Dynamic;


void dyn_pool_init(MemoryPool_Dynamic** owner, unsigned size);
void* dyn_pool_get(MemoryPool_Dynamic* pool, unsigned offset);
unsigned dyn_pool_alloc(MemoryPool_Dynamic** pool, unsigned requested_size);
void dyn_pool_free(MemoryPool_Dynamic* pool, unsigned offset);
unsigned dyn_pool_alloc_set(MemoryPool_Dynamic** pool, unsigned amount, unsigned initial_value);
unsigned dyn_pool_realloc(MemoryPool_Dynamic** owner, unsigned offset, unsigned new_size);
unsigned dyn_pool_get_size(MemoryPool_Dynamic* pool, unsigned offset);


#endif
