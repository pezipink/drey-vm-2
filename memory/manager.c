#include "manager.h"
#include "..\global.h"
#include "..\datastructs\refarray.h"
MemoryPool_Fixed* int_memory = 0; // ints and pointers
MemoryPool_Fixed* ref_memory = 0;
MemoryPool_Fixed* hash_memory = 0;
MemoryPool_Fixed* kvp_memory = 0;
MemoryPool_Dynamic* dyn_memory = 0;
MemoryPool_Fixed* scope_memory = 0;
MemoryPool_Fixed* stack_memory = 0;  // pool of pools!

unsigned max_hash_id = 1;

ref* get_ref(unsigned offset)
{
  return (ref*)fixed_pool_get(ref_memory,offset);
}

void inc_refcount(memref ref)
{  
  if(!is_value(ref))
    {
      get_ref(ref.data.i)->refcount++;
    }
}
void dec_refcount(memref ref)
{
  if(!is_value(ref))
    {
      TL("decreasing refcount on type %i from %i\n", get_ref(ref.data.i)->type, get_ref(ref.data.i)->refcount);
      get_ref(ref.data.i)->refcount--;
    }
}  

memref malloc_ref(char type, unsigned targ_offset)
{
  TL("malloc_ref entry\n");
  #if DEBUG
  if(ref_memory->free_offset == -1)
    {
      /* printf("!!!!!!!!! CRITICAL EXCEPTION, RAN OUT OF REFERENCES!!!!!\n"); */
    }
  #endif
  unsigned ref_off = fixed_pool_alloc(ref_memory);
  TL("new ref offset is %i\n",ref_off);
  ref* r = (ref*)fixed_pool_get(ref_memory,ref_off);
  // ref->ref_off = ref_off;
  r->type = type;
  r->targ_off = targ_offset;
  r->refcount = 0;
  memref mr;
  mr.type = type;
  mr.data.i = ref_off;
  TL("malloc_ref exit\n");
  return mr;
}

memref malloc_kvp(memref key, memref val, memref next)
{
  TL("malloc_kvp entry\n");
  //allocate int  
  int kvp_off = fixed_pool_alloc(kvp_memory);
  key_value* kvp_val = (key_value*)fixed_pool_get(kvp_memory,kvp_off);
  kvp_val->key = key;
  kvp_val->val = val;
  kvp_val->next = next;
  //alloc ref to it
  memref ref = malloc_ref(KVP,kvp_off);
  TL("malloc_kvp exit\n");
  return ref;
}


memref malloc_int(int val)
{
  TL("malloc_int entry\n");
  //allocate int
  memref v;
  v.type = Int32;
  v.data.i = val;
  TL("malloc_int exit\n");
  return v;
}

void free_ref(ref* ref, int ref_offset)
{
  TL("free_ref enter for offset %i type %i\n", ref_offset, ref->type);
  //assume refcount is already zero
  switch(ref->type)
    {
    case String:
      DL("freeing string..\n");
      ra_string_free(ref);
      break;
    case Array:
      DL("freeing array..\n");
      ra_free(ref);
      break;
    case Hash:
      DL("freeing hash..\n");
      hash_free(ref);
      break;
    case KVP:
      DL("freeing kvp..\n");
      fixed_pool_free(kvp_memory, ref->targ_off);
      break;
    default:
      DL("didnt know how to free memory type %i\n", ref->type);
      return;
    }

  
  ref->type = 0;
  fixed_pool_free(ref_memory,ref_offset);
}

void* deref(memref* ref)
{
  switch(ref->type)
    {
    case Int32:
      return &ref->data.i;
      break;
    case Hash:
      return fixed_pool_get(hash_memory, get_ref(ref->data.i)->targ_off);
      break;
    case KVP:
      return fixed_pool_get(kvp_memory, get_ref(ref->data.i)->targ_off);
    case Array:
    case String:
      return dyn_pool_get(dyn_memory, get_ref(ref->data.i)->targ_off);
    case Scope:
      return fixed_pool_get(scope_memory, get_ref(ref->data.i)->targ_off);
    case Stack:
      return fixed_pool_get(stack_memory, get_ref(ref->data.i)->targ_off);
    default:
      DL("Dereference failed, invalid type %i\n",ref->type);
      // DL("targ off %i\n",reftarg_off);
      return 0;
      
    }

  DL("Dereference failed, invalid type %i",ref->type);
  return 0;

}

/* void* deref_off(unsigned ref_off) */
/* { */
/*   memref m; */
/*   ref* r = get_ref(ref_off); */
/*   m.type = r->type; */
/*   m.data.i = ref_off; */
/*   return deref(&m); */
/* } */



int memref_equal(memref x, memref y)
{
  switch(x.type)
    {
    case Int32:
      switch(y.type)
        {
        case Int32:
          int* iia = (int*)deref(&x);
          int* iib = (int*)deref(&y);
          if(*iia == *iib)
            {
              return 1;
            }
          else
            {
              return 0;
            }
        case Bool:
          int* iba = (int*)deref(&x);
          int* ibb = (int*)deref(&y);
          if(*iba != 0 && *ibb != 0)
            {
              return 1;
            }
          else if(*iba == 0 && *ibb == 0)
            {
              return 1;
            }
          else
            {
              return 0;
            }
          
        default:
          break;
        }

    case Bool:
      switch(y.type)
        {
        case Int32:
          int* bia = (int*)deref(&x);
          int* bib = (int*)deref(&y);
          if(*bia != 0 && *bib != 0)
            {
              return 1;
            }
          else if(*bia == 0 && *bib == 0)
            {
              return 1;
            }
          else
            {
              return 0;
            }
        case Bool:
          int* bba = (int*)deref(&x);
          int* bbb = (int*)deref(&y);
          return *bba == *bbb;
          
        default:
          break;
        }

    case String:
    case Array:
      switch(y.type)
        {
        case String:
          if(get_ref(x.data.i)->targ_off == get_ref(y.data.i)->targ_off)
            {
              return 1;
            }
          
          refarray* ss1 = deref(&x); 
          refarray* ss2 = deref(&y);
          if(ss1->element_count != ss2->element_count)
            {
              return 0;
            }
          return memcmp(&ss1->data,&ss2->data,ss1->element_count) == 0;
        default:
          break;
        }
      
    default:
      if(x.type >= Hash && y.type >= Hash && x.type == y.type)
        {
          DL("Default reference equality used for type %i\n", x.type);
          return get_ref(x.data.i)->targ_off == get_ref(y.data.i)->targ_off;
        }
      else
        {          
          DL("!!! NO EQUAL FUNCTION FOR %i and %i!\n", x.type, y.type);
        }
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

unsigned memref_hash(memref ref)
{
  unsigned hash = 42;
  switch(ref.type)
    {
    case Int32:
      unsigned x = ref.data.i;
      return x;
      break;

    case String:
      refarray* ra = deref(&ref);
      unsigned h = fnv_hash(&ra->data,sizeof(char) * ra->element_count);
      return h;

    default:
      DL("NO HASH FUNCTION FOUND FOR TYPE %i!!\n",ref.type);
      break;
    }
  return 42;
}

memref int_to_memref(int value)
{
  memref r;
  r.type = Int32;
  r.data.i = value;
  return r;
}

int is_value(memref x)
{
  return x.type < Hash;
}
