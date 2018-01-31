#include <windows.h>
#include "dynamic_pool.h"
typedef struct FreeBlock
{
  int size;
  int next_offset;
  void* data;
} FreeBlock;


typedef struct AllocatedBlock
{
  int size;
  void* data;
} AllocatedBlock;

void dyn_pool_free(MemoryPool_Dynamic* pool, int offset)
{  
  AllocatedBlock* alloc = (AllocatedBlock*)((int)&pool->data + offset);
  FreeBlock* previous = NULL;
  int previous_offset = 0;
  FreeBlock* next = (FreeBlock*)((int)&pool->data + pool->free_offset);
  int next_offset = pool->free_offset;
  while((int)next < (int)alloc && next->next_offset != -1)
    {
      previous = next;
      previous_offset = next_offset;
      next_offset = next->next_offset;
      next = (FreeBlock*)((int)&pool->data + next_offset);
    }

  int expand_offset = 0;
  FreeBlock* expand = NULL;
  //see if we can expand into the block behind us
  if(previous != NULL && (int)previous + previous->size == (int)alloc)
    {
      expand = previous;
      expand_offset = previous_offset;
      expand->size += alloc->size;
      //in this case we dont need to modify the list behind us
    }
  else
    {
      expand_offset = offset;
      expand = (FreeBlock*)alloc;
      //point the list/root behind at us
      if(previous == NULL)
        {
          pool->free_offset = expand_offset;
        }
      else
        {
          previous->next_offset = expand_offset;
        }
      //and us in front
    }
  
  expand->next_offset = next_offset;
  //see if we can expand into the block in front of us
  if(next == NULL && (int)next == (int)alloc + alloc->size)
    {
      //consume the next block and point ourself at its next
      expand->size += next->size;
      expand->next_offset = next->next_offset;
    }
  
}

int dyn_pool_alloc(MemoryPool_Dynamic* pool, int requested_size)
{
  printf("dyn_pool_alloc enter\n");
  int offset = -1;
  //we always add an extra int's worth for the size header
  int actualSize = requested_size + sizeof(int);
  // find a suitable free block.
  int current_offset = pool->free_offset;
  FreeBlock* current = (FreeBlock*)((int)&pool->data + current_offset);
  if(current == NULL)
    {
      printf("Out of memory!");
      // reallocate here.
      return offset;
      
    }
  printf("searching for free block\n");
  FreeBlock* previous = NULL;
  int previous_offset = 0;
  while(current->size < actualSize && current->next_offset != -1)
    {
      previous = current;
      previous_offset = current_offset;            
      current_offset = current->next_offset;
      current = (FreeBlock*)((int)&pool->data + current_offset);
    }
  if(current->size < actualSize)
    {
      printf("Out of memory!");
      // realloc() here
      return offset;
    }
  int next_offset = current->next_offset;
  FreeBlock* next = (FreeBlock*)((int)&pool->data + current->next_offset);
  AllocatedBlock* alloc = (AllocatedBlock*)current;
  offset = current_offset;
  if(current->size >= actualSize)
    {
      if(current->size - actualSize < 5)
        {
          // not enough room for another useful free block
          // so include all of it
#if DEBUG
          if(current->size == actualSize)
            {
              printf("exact size\n");
            }
          else
            {
              printf("expanding by %i\n",current->size - actualSize);
            }
#endif
          alloc->size = current->size;
        }
      else
        {
          //setup new free block with the remainder
          FreeBlock* newBlock = (FreeBlock*)((int)alloc + actualSize);          
          newBlock->size = current->size - actualSize;
          alloc->size = actualSize;
          newBlock->next_offset = next_offset;
          next = newBlock;
          next_offset = current_offset + actualSize;
        }
    }
  printf("here!\n");
  if(previous == NULL)
    {
      //re-assign root
      pool->free_offset = next_offset;
    }
  else
    {
      //fix list
      previous->next_offset = next_offset;
    }
  
  return offset;
  printf("dyn_pool_alloc exit\n");
}

void dyn_pool_init(MemoryPool_Dynamic** owner, int size)
{
  //todo: size shoule be -1 because of void*?
  MemoryPool_Dynamic* pool = (MemoryPool_Dynamic*)malloc(size + sizeof(MemoryPool_Dynamic));
  FreeBlock* free = (FreeBlock*)&pool->data;
  free->size = size;
  free->next_offset = -1;
  pool->free_offset = 0;
  pool->total_size = size + sizeof(MemoryPool_Dynamic);
  *owner = pool;
}

void* dyn_pool_get(MemoryPool_Dynamic* pool, int offset)
{
  int* address = (int*)((int)&pool->data + offset);
  return (void*)address;
}

int dyn_pool_alloc_set(MemoryPool_Dynamic* pool, int amount, int initial_value)
{
  int offset = dyn_pool_alloc(pool,amount);
  
  if(offset != -1)
    {
#if DEBUG
      printf("allocated %i bytes at address %p  \n",amount,offset);
#endif
      // extra int is the size header
      int* address = (int*)((int)&pool->data + offset + sizeof(int));
      memset(address ,initial_value,amount);

      //      printf("memset %i bytes at address %p  \n",amount,r);
    }
  else
    {
#if DEBUG
      printf("allocation of %i bytes failed!\n",amount);
#endif
    }
  return offset;
}

