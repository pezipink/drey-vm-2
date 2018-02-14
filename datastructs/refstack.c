#include "refstack.h"
#include "..\memory\manager.h"
#include "..\global.h"

refstack stack_init(int initialSize)
{
  TL("stack_init entry\n");
  refstack stack;
  stack.head_offset = 0;
  fixed_pool_init(&stack.pool, sizeof(memref*), initialSize);
  TL("stack_init exit\n");
  return stack;
}

memref* stack_peek(refstack* stack)
{  
  return *(memref**)fixed_pool_get(stack->pool,stack->head_offset);
}

memref* stack_pop(refstack* stack)
{
  //  assert(stack->head_offset > 0);
  memref* ref = *(memref**)fixed_pool_get(stack->pool,stack->head_offset);
  //  TL("popping stack with ref offset of %i",*ref);
  //memref* ref = (memref*)fixed_pool_get(ref_memory,*ref_off);
  ref->refcount--;
  fixed_pool_free(stack->pool,stack->head_offset);
  stack->head_offset -= sizeof(int);
  return ref;
}

void stack_push(refstack* stack, memref* ref)
{
  TL("stack_push enter\n");
  ref->refcount++;
  int offset = fixed_pool_alloc(stack->pool);
  memref** ref_off = (memref**)fixed_pool_get(stack->pool,offset);
  stack->head_offset = offset;
  *ref_off = ref;
  TL("stack_push exit\n");
 }

void stack_push_int(refstack* stack, int value)
{
  stack_push(stack,malloc_int(value));
}


/* void debug_print_stack(refstack* stack) */
/* { */
/*   int* ptr = (int*)fixed_pool_get(stack->pool,0); */
/*   DL("stack head is at offset %i\n", stack->head_offset); */
/*   int i = 0; */
/*   do */
/*     { */
/*       DL("index %i ref offset %i\n",i,*ptr); */
/*       memref* ref = (memref*)fixed_pool_get(ref_memory,*ptr); */
/*       DL("\tmemref data:\n"); */
/*       DL("\t\ttype %i\n",ref->type); */
/*       DL("\t\trefcount %i\n",ref->refcount); */
/*       DL("\t\tref_off %i\n",ref->ref_off); */
/*       DL("\t\ttarg_off %i\n",ref->targ_off); */
/*       i += sizeof(int); */
/*       ptr++; */
/*     } */
/*   while(i <= stack->head_offset); */
/* } */
