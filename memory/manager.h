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
#define GameObject 40
#define Location 41
#define LocationReference 42

typedef struct ref
{
  unsigned int targ_off;
  char type; //msb is 1 if in use (for gc)
  char flags; 
 
} ref;

typedef struct memref
{
  char type;
  union
  {
    int i;
    double d;
  } data;
} memref, stringref, hashref, raref, locref;

typedef struct key_value
{
  memref key;
  memref val;
  memref next;
} key_value;

typedef struct scope
{
  memref locals; //obj->obj dict
  int return_address; //if a func
  memref closure_scope;
  char flags;  //used by gc for cycle pruning
} scope;

typedef struct function
{
  memref closure_scope;
  int address;
} function;

typedef struct gameobject
{
  int id;
  stringref visibility;
  hashref props;
  stringref location_key;
} gameobject, go;

typedef struct location
{
  raref siblings;
  locref parent;
  raref children;
  stringref key;
  hashref props;
  hashref objects; // int->go
} location;

typedef struct locationref
{
  int id;
  stringref key;
  hashref props;
} locationref;

//extern MemoryPool_Fixed* int_memory; // ints and pointers
extern MemoryPool_Fixed* ref_memory;
extern MemoryPool_Fixed* hash_memory;
extern MemoryPool_Fixed* kvp_memory;
extern MemoryPool_Fixed* scope_memory;
extern MemoryPool_Fixed* func_memory;
extern MemoryPool_Fixed* stack_memory; // pool of pools
extern MemoryPool_Dynamic* dyn_memory;
extern MemoryPool_Fixed* go_memory;
extern MemoryPool_Fixed* loc_memory;
extern MemoryPool_Fixed* loc_ref_memory; 
extern unsigned max_hash_id;

memref malloc_ref(char type, unsigned targ_offset);
memref malloc_kvp(memref key, memref val, memref next);
memref malloc_int(int val);
memref malloc_go();
void free_ref(ref* ref, int ref_offset);

inline void* deref(memref* ref);
void* deref_off(unsigned offset);
ref* get_ref(unsigned offset);

memref int_to_memref(int value);
  
int memref_equal(memref x, memref y);

int memref_lt(memref x, memref y);
int memref_lte(memref x, memref y);
int memref_gt(memref x, memref y);
int memref_gte(memref x, memref y);

unsigned memref_hash(memref ref);

int is_value(memref x);

#endif 
