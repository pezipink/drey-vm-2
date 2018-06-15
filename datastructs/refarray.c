#include "refarray.h"
#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"


memref ra_init_raw(unsigned element_size, unsigned element_capacity, unsigned int type)
{  
  TL("ra_init entry\n");
  TL("requested block size is %i\n",element_size * element_capacity + (sizeof(refarray)-sizeof(int)));
  int ra_off = dyn_pool_alloc_set(&dyn_memory,element_size * element_capacity + (sizeof(refarray)-sizeof(int)),0);
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,ra_off);
  TL("actual size was %i\n", dyn_pool_get_size(dyn_memory, ra_off));
  ra->element_size = element_size;
  ra->element_capacity = element_capacity;
  ra->element_count = 0;
  TL("ra initialized at offset %i ele count %i\n", ra_off, ra->element_count);
  memref ra_ref = malloc_ref(type, ra_off);
  TL("ra_init exit\n");
  return ra_ref;
}

memref ra_init(unsigned element_size, unsigned element_capacity)
{  
  return ra_init_raw(element_size, element_capacity, Array);
}

void print_array(memref ra_ref)
{
  int max = ra_count(ra_ref);
  for(int i = 0; i < max; i++)
    {
      memref n = ra_nth_memref(ra_ref,i);
      printf("index %i has ref of type %i and val %i\n",i,  n.type, n.data.i);
    }
}
  

void ra_consume_capacity(memref ra_ref)
{
  //not for general use, used in hash table imp
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  ra->element_count = ra->element_capacity;
}

//creates a strng from a c-style sring
memref ra_init_str(char* str)
{
  int len = strlen(str);
  memref r = ra_init_raw(sizeof(char),len, String);
  ra_consume_capacity(r);
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(r.data.i)->targ_off);
  memcpy(&ra->data,str,len);
  return r;  
}

memref ra_init_str_raw(char* str, int len)
{
  memref r = ra_init_raw(sizeof(char),len, String);
  ra_consume_capacity(r);
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(r.data.i)->targ_off);
  memcpy(&ra->data,str,len);
  return r;  
}


unsigned ra_count(memref ra_ref)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  TL("ra count is %i\n", ra->element_count);
  return ra->element_count;
}

void* ra_data(memref ra_ref)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  return &ra->data;
}

void ra_dec_count(memref ra_ref)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  ra->element_count--;
}

void* ra_nth(memref ra_ref, unsigned nth)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DL("Critical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);
      return 0;
    }
  #endif
  int address = (int)&ra->data + (nth * ra->element_size);
  return (void*)address;
}

// conviencce function, assumes data is an int and returns it
//rather than the pointer to it
int ra_nth_int(memref ra_ref, unsigned nth)
{
  int* add = ra_nth(ra_ref, nth);
  return *add;
}

memref ra_nth_memref(memref ra_ref, unsigned nth)
{
  TL("index is %i\n",nth);
  memref* add = ra_nth(ra_ref, nth);
  return *add;
}

void ra_set(memref ra_ref, unsigned nth, void* new_element)
{
  TL("ra_set enter\n");
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DL("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);
      return;
    }
  #endif
  int address = (int)&ra->data + (nth * ra->element_size);
  TL("MEMCOPY\n");
  memcpy(address,new_element,ra->element_size);
  TL("ra_set exit\n");
}

void ra_remove(memref ra_ref, unsigned nth, unsigned amount)
{
  TL("ra_remove enter\n");
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DIE("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);
      return;
    }
  if(nth + amount > ra->element_count)
    {
      DIE("Ciritical error - array out of bounds. nth + amount is %i but count is %i\n", nth + amount, ra->element_count);
      return;

    }
  if(amount == 0)
    {
      DIE("Critical error - amount cannot be zero in ra_remove");
    }
  #endif

  //copy the elements in front down and change the element count to suit.
  if(nth + amount < ra->element_count)
    {
      int toMove = ra->element_count - (nth + amount);
      int targetAddress = (int)&ra->data + (nth * ra->element_size);
      int sourceAddress = (int)&ra->data + ((nth + amount) * ra->element_size);
      memcpy(targetAddress,sourceAddress,toMove * ra->element_size);
    }
  ra->element_count -= amount;
  
}

memref ra_split_top(memref ra_ref, unsigned nth)
{
  TL("ra_split enter\n");
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DIE("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);

    }
  if(nth == 0)
    {
      DIE("Critical error - cannot split at index zero\n");
    }
  #endif

  int splitAddress = (int)&ra->data + (nth * ra->element_size);

  int newCount = ra->element_count - nth;
  TL("new array count is %i\n", newCount);
  memref newRaRef = ra_init_raw(ra->element_size,newCount,ra_ref.type);
  //deref again incase pool was moved
  ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  ra_consume_capacity(newRaRef);
  refarray* ra2 = deref(&newRaRef);
  int targetAddress = (int)&ra2->data;
  memcpy(targetAddress,splitAddress,newCount * ra->element_size);
  ra->element_count -= newCount;
  TL("old array count is %i\n", ra->element_count);
  return newRaRef;
}
memref ra_split_bottom(memref ra_ref, unsigned nth)
{
  TL("ra_split_bottom enter\n");
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DIE("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);

    }
  if(nth == 0)
    {
      DIE("Critical error - cannot split at index zero\n");
    }
  #endif

  int newCount = ra->element_count - nth;
  TL("new array count is %i\n", newCount);
  memref newRaRef = ra_init_raw(ra->element_size,newCount,ra_ref.type);
  //deref again incase pool was moved
  ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  ra_consume_capacity(newRaRef);
  refarray* ra2 = deref(&newRaRef);
  int targetAddress = (int)&ra2->data;
  memcpy(targetAddress,&ra->data,newCount * ra->element_size);
  ra_remove(ra_ref,0,nth);
  
  TL("old array count is %i\n", ra->element_count);
  return newRaRef;
}

void ra_set_memref(memref ra_ref, unsigned nth, memref* new_element)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  #if DEBUG
  if(nth >= ra->element_count)
    {
      DL("Ciritical error - array out of bounds. nth is %i but count is %i\n", nth, ra->element_count);      
      return;
    }
  #endif
  memref* ref = (memref*)&ra->data + nth;
  //(*new_element)->refcount++;
  memcpy((int)ref,(int)new_element,ra->element_size);
}

void ra_shuffle(memref ra)
{
  // simple fischer-yates shuffle
  int len = ra_count(ra);
  int i, j;
  memref temp, temp2;
  //todo: improve this by swapping the raw memory instead of using
  //the get / set functions
  for(int i = len - 1; i > 0; i--)
    {
      j = rand() % (i + 1);
      temp = ra_nth_memref(ra, j);
      temp2 = ra_nth_memref(ra, i);
      ra_set_memref(ra, j, &temp2);
      ra_set_memref(ra, i, &temp);
    }  
}

memref ra_clone(memref ra_ref)
{
  refarray* old = deref(&ra_ref);
  memref new_ref = ra_init_raw(old->element_size, old->element_capacity, ra_ref.type);
  refarray* new = deref(&new_ref);
  old = deref(&ra_ref);
  new->element_count = old->element_count;
  memcpy(&new->data,&old->data,old->element_size * old->element_capacity);
  return new_ref;    
}

void ra_append(memref ra_ref, void* new_element)
{
  TL("ra_append enter\n");
  int targ_off = get_ref(ra_ref.data.i)->targ_off;
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,targ_off);
  TL("ref array at %p\n",ra);
  int memSize = ra->element_size * ra->element_capacity + (sizeof(refarray) - sizeof(int));
  TL("block size is %i\n",memSize);
  int usedSize = (ra->element_size * ra->element_count) + (sizeof(refarray) - sizeof(int));
  TL("used size is %i\n",usedSize);
  int remSize = memSize - usedSize;
  TL("rem size is %i el size is %i\n",remSize, ra->element_size);
  if(remSize < ra->element_size)
   {
     // reallocate double the space
     TL("_targ was %i\n",targ_off);
     unsigned newCapacity = ra->element_capacity * 2;
     
     targ_off = dyn_pool_realloc(&dyn_memory,targ_off,newCapacity * ra->element_size + (sizeof(refarray) - sizeof(int)));
     //update the ref pointer incase the memory moved
     TL("now %i\n",targ_off);
     get_ref(ra_ref.data.i)->targ_off = targ_off;
     ra = (refarray*)dyn_pool_get(dyn_memory,targ_off);
     ra->element_capacity = newCapacity;
   }

 int target = ((int)&ra->data + (ra->element_count * ra->element_size));
 ra->element_count ++;
 memcpy(target,new_element,ra->element_size);

 TL("ra_append exit\n");
 
}

void ra_append_memref(memref ra_ref, memref new_element)
{
  ra_append(ra_ref, &new_element);
}

void ra_set_int(memref ra_ref, unsigned nth, int val)
{
  memref i;
  i.type = Int32;
  i.data.i = val;
  ra_set(ra_ref,nth,&i);
}

void ra_append_int(memref ra_ref, int val)
{
  ra_append(ra_ref,&val);
}

void ra_append_char(memref ra_ref, char val)
{
  ra_append(ra_ref,&val);
}

void ra_append_str(memref ra_ref, char* val, int len)
{
  for(int i = 0; i < len; i++)
    {
      ra_append(ra_ref,val++);
    }
}

void ra_append_ra_str(memref ra_ref, memref ra_source_ref)
{
  //todo: we could memcpy this if we ensure there's enough space.
  int len = ra_count(ra_source_ref);
  ra_append_str(ra_ref, ra_data(ra_source_ref), len);
}
///prints string
void ra_wl(memref ra_ref)
{
  ra_w(ra_ref);
  putchar('\n');
}

void ra_w(memref ra_ref)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);

  char* data = (char*)&ra->data;
  for(int i = 0; i < ra->element_count; i++)
    {
      putchar(*data++);
    }
}
void ra_w_i(memref ra_ref, int indent)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);

  for(int i = 0; i < indent; i++)
    {
      putchar(' ');
    }
  char* data = (char*)&ra->data;
  for(int i = 0; i < ra->element_count; i++)
    {
      putchar(*data++);
    }
}

void ra_string_free(ref* ra_ref)
{
  int targ_off = ra_ref->targ_off;
  dyn_pool_free(dyn_memory, targ_off);
}


void ra_free(ref* ra_ref)
{
  TL("ra_free enter\n");
  int targ_off = ra_ref->targ_off;
  refarray* ra = dyn_pool_get(dyn_memory, targ_off);
  dyn_pool_free(dyn_memory, targ_off);
  TL("ra_free exit\n");
}


