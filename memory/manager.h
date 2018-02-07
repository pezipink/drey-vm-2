#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H
#include "fixed_pool.h"
#include "dynamic_pool.h"

#define Int32 1
#define Bool 2
#define Double 3
#define Hash 4
#define String 5
#define List 6
#define Function 7

typedef struct memref
{
  char type;
  short refcount;
  int ref_off;
  int targ_off;
  
} memref, stringref, intref;

extern MemoryPool_Fixed* int_memory; // ints and pointers
extern MemoryPool_Fixed* ref_memory;
extern MemoryPool_Fixed* hash_memory;
extern MemoryPool_Dynamic* dyn_memory;
extern int max_hash_id;

memref* malloc_ref(char type, int targ_offset);
memref* malloc_int(int val);

void* deref(memref* ref);

int memref_equal(memref* x, memref* y);
int memref_hash(memref* ref);

#endif 
