#include <windows.h>
#include "fixed_pool.h"
#include "..\global.h"
// memory pool of fixed size elements. minimum size of int.
// this allocator has the following properties:
//   1. Free list is maintained inside the free data at no cost
//   2. relative offsets are used meaning the entire thing can be moved with realloc()
//   3. the owner address is updated if the pool is moved.
//   4. item access is indirectly though an item offset (byte size).
//      this will be very fast - an addition and a defrerence.
//   5. no defragging is ever required since the blocks are the same size
//      either there's a free block or there's not.


unsigned fixed_pool_alloc(MemoryPool_Fixed* pool)
{
  unsigned offset = pool->free_offset;
  if(offset == -1)
    {      
      unsigned oldCount = pool->element_count;
      unsigned newCount = oldCount * 2;
      unsigned newElementSize = (newCount * pool->element_size);
      unsigned newSize =  newElementSize + sizeof(MemoryPool_Fixed);      
      TL("out of memory for pool at %p, reallocating as size %i\n", pool, newSize);
      pool = (MemoryPool_Fixed*)realloc((void*)pool,newSize);
      TL("!\n");
      TL("pool now at %p\n", pool);
      pool->element_count = newCount;
      TL("offset was %i\n", pool->free_offset);
      offset = pool->free_offset = oldCount * pool->element_size;
      TL("offset now %i\n", pool->free_offset);
      unsigned address = (int)&pool->data + offset;
      //setup linked list in the new block
      for(unsigned i = oldCount; i < newCount - 1; i++)
        {          
          offset += pool->element_size;
          *(unsigned*)address = offset;
          address += pool->element_size;
        }
      *(unsigned*)address = -1;

      //rewrite owner's reference address
      *pool->owner = pool;
      offset = pool->free_offset;
    }
        
  unsigned* element = (unsigned*)((unsigned)&pool->data + offset);
  pool->free_offset = *element;
  return offset;
}

void fixed_pool_free(MemoryPool_Fixed* pool, unsigned offset)
{
  unsigned* element = (unsigned*)((unsigned)&pool->data + offset);
  *element = pool->free_offset;
  pool->free_offset = offset;
}

void fixed_pool_init(MemoryPool_Fixed** owner, unsigned element_size, unsigned initial_element_count)
{
  unsigned actualElementSize = (element_size * initial_element_count); 
  unsigned actualSize =  actualElementSize + sizeof(MemoryPool_Fixed);
  MemoryPool_Fixed* data = (MemoryPool_Fixed*)malloc(actualSize);
  if(data)
    {
      data->free_offset = 0;
      data->element_size = element_size;
      data->element_count = initial_element_count;
      data->owner = owner;
      unsigned address = (unsigned)&data->data;
      unsigned offset = 0;
      //setup free list
      for(unsigned i = 0; i < initial_element_count - 1; i++)
        {          
          offset += element_size;

          *(unsigned*)address = offset;
          address += element_size;
        }
      *(unsigned*)address = -1;
    }
  *owner = data;
}

void* fixed_pool_get(MemoryPool_Fixed* pool, unsigned offset)
{
  unsigned* address = (unsigned*)((unsigned)&pool->data + offset);
  return (void*)address;
}


