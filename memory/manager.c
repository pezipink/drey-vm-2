#include "manager.h"
#include "..\global.h"
MemoryPool_Fixed* int_memory = 0; // ints and pointers
MemoryPool_Fixed* ref_memory = 0;
MemoryPool_Fixed* hash_memory = 0;
MemoryPool_Fixed* kvp_memory = 0;
MemoryPool_Dynamic* dyn_memory = 0;
unsigned max_hash_id = 1;

memref* get_ref(unsigned offset)
{
  return (memref*)fixed_pool_get(ref_memory,offset);
}

memref* malloc_ref(char type, unsigned targ_offset)
{
  TL("malloc_ref entry\n");
  unsigned ref_off = fixed_pool_alloc(ref_memory);
  TL("new ref offset is %i\n",ref_off);
  memref* ref = (memref*)fixed_pool_get(ref_memory,ref_off);
  // ref->ref_off = ref_off;
  ref->targ_off = targ_offset;
  ref->type = type;
  ref->refcount = 0;
  TL("malloc_ref exit\n");
  return ref;
}

memref* malloc_int(int val)
{
  TL("malloc_int entry\n");
  //allocate int
  unsigned int_off = fixed_pool_alloc(int_memory);
  int* int_val = (int*)fixed_pool_get(int_memory,int_off);
  *int_val = val;
  //alloc ref to it
  memref* ref = malloc_ref(Int32,int_off);
  TL("malloc_int exit\n");
  return ref;
}


void* deref(memref* ref)
{
  switch(ref->type)
    {
    case Int32:
      return fixed_pool_get(int_memory, ref->targ_off);
      break;
    case Hash:
      return fixed_pool_get(hash_memory, ref->targ_off);
      break;
    case KVP:
      return fixed_pool_get(kvp_memory, ref->targ_off);
    case Array:
      return dyn_pool_get(dyn_memory, ref->targ_off);
    default:
      DL("Dereference failed, invalid type %i\n",ref->type);
      DL("targ off %i\n",ref->targ_off);
      return 0;
      
    }

  DL("Dereference failed, invalid type %i",ref->type);
  return 0;

}

void* deref_off(unsigned ref_off)
{
  return deref(get_ref(ref_off));
}

int memref_equal(memref* x, memref* y)
{
  if(x->type != y->type)
    {
      return 0;
    }
  switch(x->type)
    {
    case Int32:
      int* a = (int*)deref(x);
      int* b = (int*)deref(y);
      if(*a == *b)
        {
          return 1;
        }
      else
        {
          return 0;
        }
      break;

    }

  return 0;
}
unsigned elf_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned h = 0, g;
    int i;

    for (i = 0; i < len; i++)
    {
        h = (h << 4) + p[i];
        g = h & 0xf0000000L;

        if (g != 0)
        {
            h ^= g >> 24;
        }

        h &= ~g;
    }

    return h;
}
unsigned fnv_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned h = 2166136261;
    int i;

    for (i = 0; i < len; i++)
    {
        h = (h * 16777619) ^ p[i];
    }

    return h;
}
unsigned oat_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned h = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

unsigned memref_hash(memref* ref)
{
  unsigned hash = 42;
  if(ref->type == Int32)
    {
      unsigned x = *(unsigned*)fixed_pool_get(int_memory, ref->targ_off);
      TL("passing %i to oat_hash\n", x);
      unsigned h = fnv_hash(&x,sizeof(int));
      TL("hash is %i \n",h);
      return h;

    }
  //todo
  DL("NO HASH FUNCTION FOUND!!\n");
  return 42;
}
