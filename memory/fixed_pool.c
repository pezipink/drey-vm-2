#include <windows.h>
#include "fixed_pool.h"

// memory pool of fixed size elements. minimum size of int.
// this allocator has the following properties:
//   1. Free list is maintained inside the free data at no cost
//   2. relative offsets are used meaning the entire thing can be moved with realloc()
//   3. the owner address is updated if the pool is moved.
//   4. item access is indirectly though an item offset (byte size).
//      this will be very fast - an addition and a defrerence.
//   5. no defragging is ever required since the blocks are the same size
//      either there's a free block or there's not.



int fixed_pool_alloc(MemoryPool_Fixed* pool)
{
  int offset = pool->free_offset;
  if(offset == -1)
    {      
      int oldCount = pool->element_count;
      int newCount = oldCount * 2;
      int newElementSize = (newCount * pool->element_size);
      int newSize =  newElementSize + sizeof(MemoryPool_Fixed);      
      printf("out of memory, reallocating as size %i\n", newSize);
      pool = (MemoryPool_Fixed*)realloc((void*)pool,newSize);
      pool->element_count = newCount;
      offset = pool->free_offset = (oldCount + 1) * pool->element_size;
      int address = (int)&pool->data + offset;
      //setup linked list in the new block
      for(int i = oldCount; i < newCount - 1; i++)
        {          
          offset += pool->element_size;
          *(int*)address = offset;
          address += pool->element_size;
        }
      *(int*)address = -1;

      //rewrite owner's reference address
      *pool->owner = pool;
      offset = pool->free_offset;
    }
        
  int* element = (int*)((int)&pool->data + offset);
  pool->free_offset = *element;
  return offset;
}

void fixed_pool_free(MemoryPool_Fixed* pool, int offset)
{
  int* element = (int*)((int)&pool->data + offset);
  *element = pool->free_offset;
  pool->free_offset = offset;
}

void fixed_pool_init(MemoryPool_Fixed** owner, int element_size, int initial_element_count)
{
  int actualElementSize = (element_size * initial_element_count); 
  int actualSize =  actualElementSize + sizeof(MemoryPool_Fixed);
  MemoryPool_Fixed* data = (MemoryPool_Fixed*)malloc(actualSize);
  if(data)
    {
      data->free_offset = 0;
      data->element_size = element_size;
      data->element_count = initial_element_count;
      data->owner = owner;
      int address = (int)&data->data;
      int offset = 0;
      //setup free list
      for(int i = 0; i < initial_element_count - 1; i++)
        {          
          offset += element_size;
          *(int*)address = offset;
          address += element_size;
        }
      *(int*)address = -1;
    }
  *owner = data;
}

void* fixed_pool_get(MemoryPool_Fixed* pool, int offset)
{
  int* address = (int*)((int)&pool->data + offset);
  return (void*)address;
}


