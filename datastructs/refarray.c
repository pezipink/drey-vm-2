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

unsigned ra_count(memref ra_ref)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
  return ra->element_count;
}

void* ra_nth(memref ra_ref, unsigned nth)
{
  refarray* ra = (refarray*)dyn_pool_get(dyn_memory,get_ref(ra_ref.data.i)->targ_off);
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
  memref* ref = (memref*)&ra->data + (nth * ra->element_size);
  if(ref != 0 && ref->type > 3)
    {
      get_ref(ref->data.i)->refcount--;
    }
  //(*new_element)->refcount++;
  memcpy((int)ref,(int)new_element,ra->element_size);
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
  inc_refcount(new_element);
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

  // for any reference items, decrease their refcount.

  //special case here for KVP items, since they can only appear
  //in an RA and form a linked list, also traverse that and adjust
  //the refcounts.
  int max = ra->element_count;
  memref* data = (memref*)&ra->data;
  for(int i = 0; i < max; i++)
    {
      //    DL("!!TESTTING type %i\n", data->type);
      if(data->type == KVP)
        {
          dec_refcount(*data);
          DL("KVP\n");
          key_value* kvp = deref(data);
          while(1)
            {
              dec_refcount(kvp->key);
              dec_refcount(kvp->val);
              if(kvp->next.type != 0)
                {
                  kvp = deref(&kvp->next);
                }
              else
                {
                  break;
                }
            }
        }
      else if(!is_value(*data))
        {
          dec_refcount(*data);
        }
      data++;
    }  

  
  dyn_pool_free(dyn_memory, targ_off);
  TL("ra_free exit\n");
}
