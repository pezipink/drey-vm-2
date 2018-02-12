#include <windows.h>
#include "dynamic_pool.h"
#include "..\global.h"
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

// finds the free blocks before and after the allocated block,
// or null if they don't exist.
void find_adjacent_blocks(MemoryPool_Dynamic* pool,
                          AllocatedBlock* alloc,
                          FreeBlock** previous,
                          int* previous_offset,
                          FreeBlock** next,
                          int* next_offset)
{
  *previous = NULL;
  *previous_offset = 0;
  *next = NULL;
  *next_offset = pool->free_offset;
  if(*next_offset != -1)
    {
      *next = (FreeBlock*)((int)&pool->data + pool->free_offset);  
      while((int)*next < (int)alloc && (*next)->next_offset != -1)
        {
          *previous = *next;
          *previous_offset = *next_offset;
          *next_offset = (*next)->next_offset;
          *next = (FreeBlock*)((int)&pool->data + *next_offset);
        }
    }
}

void dyn_pool_free(MemoryPool_Dynamic* pool, int offset)
{
  TL("entered dyn_pool_free at %p for offset %i\n", pool, offset);
  AllocatedBlock* alloc = (AllocatedBlock*)((int)&pool->data + offset);
  FreeBlock *previous, *next;
  int previous_offset, next_offset;
  find_adjacent_blocks(pool,alloc,&previous,&previous_offset,&next,&next_offset);

  int expand_offset = 0;
  FreeBlock* expand = NULL;
  //see if we can expand into the block behind us
  if(previous != NULL && (int)previous + previous->size == (int)alloc)
    {
      TL("expanding into previous block at offset %i\n", previous_offset);
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
      TL("expanding into next block at offset %i\n",next_offset);
      //consume the next block and point ourself at its next
      expand->size += next->size;
      expand->next_offset = next->next_offset;
    }
  TL("exit dyn_pool_free\n");
}


int dyn_pool_alloc_internal(MemoryPool_Dynamic* pool, int requested_size)
{
  TL("dyn_pool_alloc_internal enter %p\n", pool);
  int offset = -1;
  //we always add an extra int's worth for the size header
  int actual_size = requested_size + sizeof(int);
  // find a suitable free block.
  int current_offset = pool->free_offset;
  TL("first free offset is %i\n", pool->free_offset);

  if(current_offset == -1)
    {
      TL("Out of memory!\n");
      return offset;
    }
  TL("searching for free block of size %i\n", actual_size);
  FreeBlock* current = (FreeBlock*)((int)&pool->data + current_offset);
  TL("first free block size %i\n", current->size);
  FreeBlock* previous = NULL;
  int previous_offset = 0;
  while(current->size < actual_size && current->next_offset != -1)
    {
      previous = current;
      previous_offset = current_offset;            
      current_offset = current->next_offset;
      current = (FreeBlock*)((int)&pool->data + current_offset);
      TL("current free block offset %i size %i\n", current_offset, current->size);
    }
  if(current->size < actual_size)
    {
      TL("Block not big enough .. Out of memory!\n");
      return offset;
    }
  int next_offset = current->next_offset;
  FreeBlock* next = (FreeBlock*)((int)&pool->data + current->next_offset);
  AllocatedBlock* alloc = (AllocatedBlock*)current;
  
  offset = current_offset;
  TL("allocated block offset %i\n", offset);
  if(current->size >= actual_size)
    {
      if(current->size - actual_size < 5)
        {
          // not enough room for another useful free block
          // so include all of it
#if DEBUG
          if(current->size == actual_size)
            {
              TL("exact size\n");
            }
          else
            {
              TL("expanding by %i\n",current->size - actual_size);
            }
#endif
          alloc->size = current->size;
        }
      else
        {
          //setup new free block with the remainder
          FreeBlock* newBlock = (FreeBlock*)((int)alloc + actual_size);          
          newBlock->size = current->size - actual_size;
          alloc->size = actual_size;
          newBlock->next_offset = next_offset;
          next = newBlock;
          next_offset = current_offset + actual_size;
        }
    }

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
  TL("dyn_pool_alloc_internal exit\n");
}

int dyn_pool_alloc(MemoryPool_Dynamic** pool, int requested_size)
{
  TL("dyn_pool_alloc enter %p\n", *pool);
  int offset = dyn_pool_alloc_internal(*pool, requested_size);
  if(offset > -1 )
    {
      return offset;
    }

  if(dyn_pool_resize(*pool, requested_size + sizeof(int)))
    {
      offset = dyn_pool_alloc_internal(*pool, requested_size);
      if(offset > -1 )
        {
          return offset;
        }
      else
        {
          TL("WTF\n!");
        }     
    }
  else
    {
      TL("critical error, dynamic pool reallocation failed\n");
      return -1;
    }
  return -1;
}

void dyn_pool_init(MemoryPool_Dynamic** owner, int size)
{
  //todo: size shoule be -1 because of void*?
  MemoryPool_Dynamic* pool = (MemoryPool_Dynamic*)malloc(size + sizeof(MemoryPool_Dynamic));
  FreeBlock* free = (FreeBlock*)&pool->data;
  free->size = size;
  free->next_offset = -1;
  pool->free_offset = 0;
  // do not include the size of the data void pointer 
  pool->total_size = size + sizeof(MemoryPool_Dynamic) - sizeof(int);
  pool->owner = owner;
  *owner = pool;
}

bool dyn_pool_resize(MemoryPool_Dynamic* pool, int requested_block_size)
{
  TL("dyn_pool_resize enter %p\n", pool);
  //reallocate here. resize to double, or double the size + request size
  int new_total_size = 0;
  int new_block_size = 0;
  int actual_size = requested_block_size + sizeof(int);
  if(actual_size > pool->total_size)
    {
      new_block_size = actual_size * 2;
      new_total_size = pool->total_size + new_block_size;
    }
  else
    {
      new_block_size = pool->total_size;
      new_total_size = pool->total_size * 2;
    }
  TL("pool at %p was %i, extending to %i\n",pool, pool->total_size, new_total_size);
  pool = (MemoryPool_Dynamic*)realloc((void*)pool,new_total_size);
  TL("pool is now at %p total size still %i\n", pool, pool->total_size);
  if(pool == NULL)
    {
      return false;
    }
  
  if(pool->free_offset == -1)
    {
      TL("memory totally full, adding new block to end\n");
      //easy case, no free blocks at all so we can just slap one on the end
      //and make it the first offset.
      FreeBlock* new_block = (FreeBlock*)((int)pool + pool->total_size - sizeof(int));
      TL("new block is located at %p data starts at %p\n", (void*)new_block, &pool->data);
      new_block->next_offset = -1;
      new_block->size = new_block_size;
      pool->free_offset = pool->total_size - sizeof(MemoryPool_Dynamic);
    }
  else
    {
      //find the last free block - if it is at the end of the memory, we will simply extend it
      // otherwise insert a new free block into the list with the new memory

      // get last block
      int current_offset = pool->free_offset;
      FreeBlock* current = (FreeBlock*)((int)&pool->data + current_offset);
      while( current->next_offset != -1)
        {
          current = (FreeBlock*)((int)&pool->data + current->next_offset);
        }
      if((int)current + current->size == (int)pool + pool->total_size)
        {
          TL("extending last free block to new size\n");
          current->size += new_block_size;
        }
      else
        {
          TL("adding new block to the end of the list\n");
          FreeBlock* new_block = (FreeBlock*)((int)pool + pool->total_size - sizeof(int));
          TL("new block is located at %p data starts at %p\n", (void*)new_block, &pool->data);
          new_block->next_offset = -1;
          new_block->size = new_block_size;
          pool->free_offset = pool->total_size - sizeof(MemoryPool_Dynamic);
          current->next_offset = pool->total_size - sizeof(MemoryPool_Dynamic);
        }
      

    }
  pool->total_size = new_total_size;
  TL("new pool size is %i\n", pool->total_size);
  TL("new pool free offset is %i\n", pool->free_offset);
  TL("owner was %p\n", *pool->owner);
  *pool->owner = pool;
  TL("owner now %p\n", *pool->owner);
  TL("dyn_pool_resize exit\n");
  return true;
}
                          


int dyn_pool_realloc(MemoryPool_Dynamic** owner, int offset, int new_size)
{
  MemoryPool_Dynamic* pool = *owner;
  //for now there's no need to support downsizing
  AllocatedBlock* alloc = (AllocatedBlock*)((int)&pool->data + offset);
  if(alloc->size > new_size)
    {
      TL("warning: attempted to donwnsize block\n");
      return offset;
    }
          
  int actual_size = new_size + sizeof(int);
  //first see if we can extend the existing block to the new size
  int retOffset = offset;
  TL("current size is %i, attempting to allocate %i to offset %i\n",alloc->size, actual_size, offset);
  FreeBlock *previous, *next;
  int previous_offset, next_offset;
  find_adjacent_blocks(pool,alloc,&previous,&previous_offset,&next,&next_offset);
  TL("the free block in front is size %i\n", next == NULL ? -1 : next->size);
  if(next != NULL && (int)next == (int)alloc + alloc->size && (next->size >= actual_size - alloc->size))
    {          
      //it is big enough.  if the remaining size is not big enough
      //to fit a new free block in, then take all of it, otherwise
      //add a new block and patch up list as appropriate.
      int rem = (alloc->size + next->size) - actual_size;
      TL("block big enough with a remainder of %i\n", rem);
      if(rem > sizeof(FreeBlock) - sizeof(int))
        {          
          FreeBlock* newBlock = (FreeBlock*)((int)alloc + actual_size);
          
          //grab these up front as they might be overlapping with
          //the new block! todo: might need to write this bit in asm
          //if the optimizer breaks it?
          int oldSize = next->size;
          int oldOffset = next->next_offset;
          int sizeDiff = oldSize - actual_size;
          int offsetDelta = actual_size - alloc->size;
          newBlock->size = sizeDiff;
          alloc->size = actual_size;
          newBlock->next_offset = oldOffset;
          if(previous == NULL)
            {
              TL("setting pool's free offset += %i\n", offsetDelta);  
              pool->free_offset += offsetDelta;
            }
          else
            {
              TL("setting previous block's free offset += %i\n", offsetDelta);
              previous->next_offset += offsetDelta;
            }
        }
      else
        {
          TL("block is not big enough for a new block, assigning all %i\n ", next->size);
          
          // gobble up the memory and set the previous
          // offset pointer to whatever was in front
          if(previous == NULL)
            {
              pool->free_offset = next->next_offset;
            }
          else
            {
              previous->next_offset = next->next_offset;
            }
          alloc->size += next->size;
        }      
    }
  else
    {
      TL("next block not big enough.  allocating new space..\n");
      //block was not big enough. get a block that is big enough, maybe causing
      //a pool resize.  copy data and free old block, checking first if the new block
      //is at the end of the existing block in which case we can extend it.
      retOffset = dyn_pool_alloc_set(owner,actual_size,0);
      pool = *owner;
      AllocatedBlock* newAlloc = (AllocatedBlock*)((int)&pool->data + retOffset);

      //todo: extend block if its the one at the end.
      TL("new space is at offset %i \n", retOffset);
      //copy data and free old block.
      memcpy(&newAlloc->data,&alloc->data,alloc->size - sizeof(int));
      dyn_pool_free(pool, offset);
      

      
    }
  return retOffset;
}

void* dyn_pool_get(MemoryPool_Dynamic* pool, int offset)
{
  AllocatedBlock* address = (AllocatedBlock*)((int)&pool->data + offset);
  return &address->data;
}

int dyn_pool_get_size(MemoryPool_Dynamic* pool, int offset)
{
  AllocatedBlock* address = (AllocatedBlock*)((int)&pool->data + offset);
  return address->size;
}

int dyn_pool_alloc_set(MemoryPool_Dynamic** pool, int amount, int initial_value)
{
  int offset = dyn_pool_alloc(pool,amount);
  
  if(offset != -1)
    {
      TL("allocated %i bytes at address %p  \n",amount,offset);
      // extra int is the size header
      MemoryPool_Dynamic* p = *pool;
      int* address = (int*)((int)&p->data + offset + sizeof(int));
      memset(address ,initial_value,amount);

    }
  else
    {
      TL("allocation of %i bytes failed!\n",amount);
    }
  return offset;
}


