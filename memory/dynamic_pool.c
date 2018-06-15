#include <windows.h>
#include "dynamic_pool.h"
#include "..\global.h"
typedef struct FreeBlock
{
  unsigned size;
  unsigned next_offset;
  void* data;
} FreeBlock;

typedef struct AllocatedBlock
{
  unsigned size;
  void* data;
} AllocatedBlock;

// finds the free blocks before and after the allocated block,
// or null if they don't exist.
void find_adjacent_blocks(MemoryPool_Dynamic* pool,
                          AllocatedBlock* alloc,
                          FreeBlock** previous,
                          unsigned* previous_offset,
                          FreeBlock** next,
                          unsigned* next_offset)
{
  *previous = NULL;
  *previous_offset = 0;
  *next = NULL;
  *next_offset = pool->free_offset;
  if(*next_offset != 0xFFFFFFFF)
    {
      *next = (FreeBlock*)((unsigned)&pool->data + pool->free_offset);
      while((unsigned)*next < (unsigned)alloc && (*next)->next_offset != 0xFFFFFFFF)
        {
          *previous = *next;
          *previous_offset = *next_offset;
          *next_offset = (*next)->next_offset;
          *next = (FreeBlock*)((unsigned)&pool->data + *next_offset);
        }
    }
}

void dyn_pool_free(MemoryPool_Dynamic* pool, unsigned offset)
{
  TL("entered dyn_pool_free at %p for offset %i\n", pool, offset);
  AllocatedBlock* alloc = (AllocatedBlock*)((unsigned)&pool->data + offset);
  FreeBlock *previous, *next;
  unsigned previous_offset, next_offset;
  find_adjacent_blocks(pool,alloc,&previous,&previous_offset,&next,&next_offset);

  unsigned expand_offset = 0;
  FreeBlock* expand = NULL;
  //see if we can expand unsignedo the block behind us
  if(previous != NULL && (unsigned)previous + previous->size == (unsigned)alloc)
    {
      TL("expanding unsignedo previous block at offset %i\n", previous_offset);
      expand = previous;
      expand_offset = previous_offset;
      expand->size += alloc->size;
      //in this case we dont need to modify the list behind us
    }
  else
    {
      expand_offset = offset;
      expand = (FreeBlock*)alloc;
      //pounsigned the list/root behind at us
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
  //see if we can expand unsignedo the block in front of us
  if(next == NULL && (unsigned)next == (unsigned)alloc + alloc->size)
    {
      TL("expanding into next block at offset %i\n",next_offset);
      //consume the next block and pounsigned ourself at its next
      expand->size += next->size;
      expand->next_offset = next->next_offset;
    }
  TL("exit dyn_pool_free\n");
}


unsigned dyn_pool_alloc_internal(MemoryPool_Dynamic* pool, unsigned requested_size)
{
  TL("dyn_pool_alloc_unsignedernal enter %p\n", pool);
  unsigned offset = 0xFFFFFFFF;
  //we always add an extra unsigned's worth for the size header
  unsigned actual_size = requested_size + sizeof(unsigned);
  // find a suitable free block.
  unsigned current_offset = pool->free_offset;
  TL("first free offset is %i\n", pool->free_offset);
  if(current_offset == 0xFFFFFFFF)
    {
      TL("Out of memory!\n");
      return offset;
    }
  TL("searching for free block of size %i\n", actual_size);
  FreeBlock* current = (FreeBlock*)((unsigned)&pool->data + current_offset);
  TL("first free block size %i\n", current->size);
  FreeBlock* previous = NULL;
  unsigned previous_offset = 0;
  while(current->size < actual_size && current->next_offset != 0xFFFFFFFF)
    {
      previous = current;
      previous_offset = current_offset;            
      current_offset = current->next_offset;
      current = (FreeBlock*)((unsigned)&pool->data + current_offset);
      TL("current free block offset %i size %i\n", current_offset, current->size);
    }
  if(current->size < actual_size)
    {
      TL("Block not big enough .. Out of memory!\n");
      return offset;
    }
  unsigned next_offset = current->next_offset;
  FreeBlock* next = (FreeBlock*)((unsigned)&pool->data + current->next_offset);
  AllocatedBlock* alloc = (AllocatedBlock*)current;
  
  offset = current_offset;
  TL("allocated block offset %i\n", offset);

  if(current->size >= actual_size)
    {
      if(current->size - actual_size <= sizeof(FreeBlock) )
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
          FreeBlock* newBlock = (FreeBlock*)((unsigned)alloc + actual_size);          
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
  TL("dyn_pool_alloc_unsignedernal exit\n");
}

unsigned dyn_pool_alloc(MemoryPool_Dynamic** pool, unsigned requested_size)
{
  TL("dyn_pool_alloc enter %p\n", *pool);
  unsigned offset = dyn_pool_alloc_internal(*pool, requested_size);

  if(offset != 0xFFFFFFFF )
    {
      return offset;
    }

  TL("resize\n");
  if(dyn_pool_resize(*pool, requested_size + sizeof(unsigned)))
    {
      TL("pool now at %p\n",pool);
      offset = dyn_pool_alloc_internal(*pool, requested_size);
      if(offset != 0xFFFFFFFF )
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
      return 0xFFFFFFFF;
    }
  return 0xFFFFFFFF;
}

void dyn_pool_init(MemoryPool_Dynamic** owner, unsigned size)
{
  //todo: size shoule be -1 because of void*?
  MemoryPool_Dynamic* pool = (MemoryPool_Dynamic*)malloc(size + sizeof(MemoryPool_Dynamic));
  FreeBlock* free = (FreeBlock*)&pool->data;
  free->size = size;
  free->next_offset = 0xFFFFFFFF;
  pool->free_offset = 0;
  // do not include the size of the data void pointer 
  pool->total_size = size + sizeof(MemoryPool_Dynamic) - sizeof(unsigned);
  pool->owner = owner;
  *owner = pool;
}

bool dyn_pool_resize(MemoryPool_Dynamic* pool, unsigned requested_block_size)
{
  TL("dyn_pool_resize enter %p\n", pool);
  //reallocate here. resize to double, or double the size + request size
  unsigned new_total_size = 0;
  unsigned new_block_size = 0;
  unsigned actual_size = requested_block_size + sizeof(unsigned);
  if(actual_size > pool->total_size)
    {
      TL("requsted block bigger than entire pool, reszing to double the requested size %u.\n", actual_size * 2);
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
      DL("!!!!critical expception, realloc returned null\n");
      return false;
    }
  
  if(pool->free_offset == 0xFFFFFFFF)
    {
      TL("memory totally full, adding new block to end\n");
      //easy case, no free blocks at all so we can just slap one on the end
      //and make it the first offset.
      FreeBlock* new_block = (FreeBlock*)((unsigned)pool + pool->total_size - sizeof(unsigned));
      TL("new block is located at %p data starts at %p\n", (void*)new_block, &pool->data);
      new_block->next_offset = 0xFFFFFFFF;
      new_block->size = new_block_size;
      pool->free_offset = pool->total_size - sizeof(MemoryPool_Dynamic);
    }
  else
    {
      //find the last free block - if it is at the end of the memory, we will simply extend it
      // otherwise insert a new free block unsignedo the list with the new memory

      // get last block
      unsigned current_offset = pool->free_offset;
      FreeBlock* current = (FreeBlock*)((unsigned)&pool->data + current_offset);
      while( current->next_offset != 0xFFFFFFFF)
        {
          current = (FreeBlock*)((unsigned)&pool->data + current->next_offset);
        }
      if((unsigned)current + current->size == (unsigned)pool + pool->total_size)
        {
          TL("extending last free block to new size\n");
          current->size += new_block_size;
        }
      else
        {
          TL("adding new block to the end of the list\n");
          FreeBlock* new_block = (FreeBlock*)((unsigned)pool + pool->total_size - sizeof(unsigned));
          TL("new block is located at %p data starts at %p\n", (void*)new_block, &pool->data);
          new_block->next_offset = 0xFFFFFFFF;
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
                          


unsigned dyn_pool_realloc(MemoryPool_Dynamic** owner, unsigned offset, unsigned new_size)
{
  MemoryPool_Dynamic* pool = *owner;
  //for now there's no need to support downsizing
  AllocatedBlock* alloc = (AllocatedBlock*)((unsigned)&pool->data + offset);
  if(alloc->size > new_size)
    {
      TL("warning: attempted to donwnsize bloc from %i to %i\n",alloc->size, new_size);
      return offset;
    }
          
  unsigned actual_size = new_size + sizeof(unsigned);
  //first see if we can extend the existing block to the new size
  unsigned retOffset = offset;
  TL("current size is %i, attempting to allocate %i to offset %i\n",alloc->size, actual_size, offset);
  TL("first free block is offset %i\n",pool->free_offset);
  FreeBlock *previous, *next;
  unsigned previous_offset, next_offset;
  find_adjacent_blocks(pool,alloc,&previous,&previous_offset,&next,&next_offset);
  TL("the free block in front is size %u\n", next == NULL ? 0xFFFFFFFF : next->size);
  if(next != NULL && (unsigned)next == (unsigned)alloc + alloc->size && (next->size >= actual_size - alloc->size))
    {          
      //it is big enough.  if the remaining size is not big enough
      //to fit a new free block in, then take all of it, otherwise
      //add a new block and patch up list as appropriate.
      unsigned rem = (alloc->size + next->size) - actual_size;
      TL("block big enough with a remainder of %u\n", rem);
      if(rem > sizeof(FreeBlock) - sizeof(unsigned))
        {
          TL("current bock adress %p\n",next);
          FreeBlock* newBlock = (FreeBlock*)((unsigned)alloc + actual_size);
          TL("new bock adress %p\n",newBlock);
          //grab these up front as they might be overlapping with
          //the new block! todo: might need to write this bit in asm
          //if the optimizer breaks it?
          unsigned oldSize = next->size;
          unsigned oldOffset = next->next_offset;
          TL("old next %i\n", next->next_offset);
          unsigned offsetDelta = actual_size - alloc->size;
          newBlock->size = rem;
          TL("new vloxk aize  %i\n", newBlock->size);
          alloc->size = actual_size;
          newBlock->next_offset = oldOffset;
          TL("new next %i\n", newBlock->next_offset);
          if(previous == NULL)
            {
              TL("setting pool's free offset += %i\n", offsetDelta);  
              pool->free_offset += offsetDelta;
              TL("pool'd offset is now %i\n", pool->free_offset);
              FreeBlock* test = (FreeBlock*)((unsigned)&pool->data + pool->free_offset);
              TL("test bock adress %p\n",test);
              TL("test block size = %i next = %i\n", test->size, test->next_offset);
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
          // offset pounsigneder to whatever was in front
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
      //is at the end of the existing block in which case we can extend it. (todo)
      TL("pool at %p\n", owner);
      retOffset = dyn_pool_alloc_set(owner,actual_size,0);
      pool = *owner;
      TL("pool now at %p\n", owner);
      AllocatedBlock* newAlloc = (AllocatedBlock*)((unsigned)&pool->data + retOffset);
      // alloc'd block might have moved and been reclaimed by the os!
      alloc = (AllocatedBlock*)((unsigned)&pool->data + offset);
      //todo: extend block if its the one at the end.
      TL("new space is at offset %i \n", retOffset);
      //copy data and free old block.
      memcpy(&newAlloc->data,&alloc->data,alloc->size - sizeof(unsigned));
      TL("data copied\n");
      dyn_pool_free(pool, offset);
      TL("data freed\n");

      
    }
  return retOffset;
}

void* dyn_pool_get(MemoryPool_Dynamic* pool, unsigned offset)
{
  AllocatedBlock* address = (AllocatedBlock*)((unsigned)&pool->data + offset);
  return &address->data;
}

unsigned dyn_pool_get_size(MemoryPool_Dynamic* pool, unsigned offset)
{
  AllocatedBlock* address = (AllocatedBlock*)((unsigned)&pool->data + offset);
  return address->size;
}

unsigned dyn_pool_alloc_set(MemoryPool_Dynamic** pool, unsigned amount, unsigned initial_value)
{
  unsigned offset = dyn_pool_alloc(pool,amount);
  
  if(offset != -1)
    {
      TL("allocated %i bytes at address %p  \n",amount,offset);
      // extra unsigned is the size header
      MemoryPool_Dynamic* p = *pool;
      unsigned* address = (unsigned*)((unsigned)&p->data + offset + sizeof(unsigned));
      memset(address ,initial_value,amount);

    }
  else
    {
      TL("allocation of %i bytes failed!\n",amount);
    }
  return offset;
}


