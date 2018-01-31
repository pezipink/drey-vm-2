#ifndef _DYNAMIC_POOL_H
#define _DYNAMIC_POOL_H

typedef struct MemoryPool_Dynamic
{
  struct MemoryPool_Dynamic** owner;
  int total_size;
  int free_offset;
  void* data;
} MemoryPool_Dynamic;


void dyn_pool_init(MemoryPool_Dynamic** owner, int size);
void* dyn_pool_get(MemoryPool_Dynamic* pool, int offset);
int dyn_pool_alloc(MemoryPool_Dynamic* pool, int requested_size);
void dyn_pool_free(MemoryPool_Dynamic* pool, int offset);




#endif
