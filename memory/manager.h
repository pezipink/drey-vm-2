#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H
#include "fixed_pool.h"
#include "dynamic_pool.h"

//value
#define Int32 1
#define Bool 2
#define Double 3
#define ExecContext 4

//ref
#define Hash 32
#define String 33
#define List 34
#define Function 35
#define Array 36
#define KVP 37
#define Scope 38
#define Stack 39

typedef struct ref
{
  char type;
  short refcount;
  unsigned targ_off;
} ref;

typedef struct memref
{
  char type;
  union
  {
    struct ref* r;
    int i;
    double d;

  } data;
} memref, stringref;

typedef struct key_value
{
  memref key;
  memref val;
  memref next;
} key_value;

extern MemoryPool_Fixed* int_memory; // ints and pointers
extern MemoryPool_Fixed* ref_memory;
extern MemoryPool_Fixed* hash_memory;
extern MemoryPool_Fixed* kvp_memory;
extern MemoryPool_Fixed* scope_memory;
extern MemoryPool_Fixed* stack_memory; // pool of pools
extern MemoryPool_Dynamic* dyn_memory;
extern unsigned max_hash_id;

memref malloc_ref(char type, unsigned targ_offset);
memref malloc_kvp(memref key, memref val, memref next);
memref malloc_int(int val);

void* deref(memref* ref);
void* deref_off(unsigned offset);
ref* get_ref(unsigned offset);

void inc_refcount(memref ref);
void dec_refcount(memref ref);

memref int_to_memref(int value);
  
int memref_equal(memref x, memref y);
unsigned memref_hash(memref ref);

int is_value(memref x);

#endif 
