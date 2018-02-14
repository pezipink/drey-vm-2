#include "refarray.h"
#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"

memref* ra_init(unsigned element_size, unsigned element_count)
{  
  TL("ra_init entry\n");
  int ra_off = dyn_pool_alloc_set(&dyn_memory,element_size * element_count + (sizeof(refarray)-sizeof(int)),0);
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,ra_off);
  ra->element_size = element_size;
  ra->element_count = element_count;
  TL("ra initialized at offset %i ele count %i\n", ra_off, ra->element_count);
  memref* ra_ref = malloc_ref(Array, ra_off);
  
  TL("ra_init exit\n");
 
  return ra_ref;
}

unsigned ra_count(memref* ra_ref)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,ra_ref->targ_off);
  return ra->element_count;
}

void* ra_nth(memref* ra_ref, unsigned nth)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,ra_ref->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DL("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);
      return 0;
    }
  #endif
  int address = (int)&ra->data + (nth * ra->element_size);
  return (void*)address;
}

// conviencce function, assumes data is an int and returns it
//rather than the pointer to it
int ra_nth_int(memref* ra_ref, unsigned nth)
{
  int* add = ra_nth(ra_ref, nth);
  return *add;
}

memref* ra_nth_memref(memref* ra_ref, unsigned nth)
{
  memref** add = ra_nth(ra_ref, nth);
  return *add;
}

void ra_set(memref* ra_ref, unsigned nth, void* new_element)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,ra_ref->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DL("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);
      return;
    }
  #endif
  int address = (int)&ra->data + (nth * ra->element_size);
  memcpy(address,new_element,ra->element_size);
}

void ra_set_memref(memref* ra_ref, unsigned nth, memref** new_element)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,ra_ref->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DL("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);      
      return;
    }
  #endif
  memref* ref = (memref*)&ra->data + (nth * ra->element_size);
  if(ref != 0)
    {
      ref->refcount--;
    }
  (*new_element)->refcount++;
  memcpy((int)ref,(int)new_element,ra->element_size);
}

void ra_append(memref** ra_ref, void* new_element)
{
  TL("ra_append enter\n");
  int targ_off = (*ra_ref)->targ_off;
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,targ_off);
  int memSize = dyn_pool_get_size(dyn_memory,targ_off);
  TL("block size is %i\n",memSize);
 // total size includes 1 int for the memory block size itself and another 2 for the element size and count
  int usedSize = (ra->element_size * ra->element_count) + (sizeof(int) * 3);
  TL("used size is %i\n",usedSize);
  int remSize = memSize - usedSize;
  TL("rem size is %i\n",remSize);
  if(remSize < ra->element_size)
   {
     // reallocate double the space
     TL("targ was %i\n",targ_off); 
     targ_off = dyn_pool_realloc(&dyn_memory,targ_off,memSize * 2);
     //update the ref pointer incase the memory moved
     TL("now %i\n",targ_off); 
     (*ra_ref)->targ_off = targ_off;
     ra = (refarray*)dyn_pool_get(dyn_memory,targ_off);
   }

 int target = ((int)&ra->data + (ra->element_count * ra->element_size));
 ra->element_count ++;
 memcpy(target,new_element,ra->element_size);

 TL("ra_append exit\n");
 
}

void ra_append_memref(memref** ra_ref, memref** new_element)
{
  ra_append(ra_ref, new_element);
  (*new_element)->refcount++;
}
