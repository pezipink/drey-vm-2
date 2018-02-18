#include "refstack.h"
#include "..\memory\manager.h"
#include "..\global.h"

memref stack_init(int initialSize)
{
  TL("stack_init entry\n");
  int off = fixed_pool_alloc(stack_memory);
  refstack* stack = (refstack*)fixed_pool_get(stack_memory,off);
  stack->head_offset = 0;  
  fixed_pool_init(&stack->pool, sizeof(memref), initialSize);
  TL("stack_init exit\n");
  memref ref = malloc_ref(Stack,off);
  return ref;
}

memref stack_peek(memref stack_ref)
{
  refstack* stack = deref(&stack_ref);
  return *(memref*)fixed_pool_get(stack->pool,stack->head_offset);
}

memref stack_pop(memref stack_ref)
{
  TL("stack_pop entry\n");
  refstack* stack = deref(&stack_ref);
  //  assert(stack->head_offset > 0);
  #if DEBUG
  if(stack->head_offset == 0xFFFFFFFF)
    {
      DL("!!!attempted to pop stack with no values !!!!\n");
    }
  #endif
  memref ref = *(memref*)fixed_pool_get(stack->pool,stack->head_offset);
  //TL("popping stack with ref offset of %i",*ref);  
  //memref* ref = (memref*)fixed_pool_get(ref_memory,*ref_off);
  dec_refcount(ref);
  fixed_pool_free(stack->pool,stack->head_offset);
  stack->head_offset -= sizeof(memref);
  TL("stack_pop exit\n");
  return ref;
}

void stack_push(memref stack_ref, memref ref)
{
  TL("stack_push enter\n");
  refstack* stack = deref(&stack_ref);
  inc_refcount(ref);
  int offset = fixed_pool_alloc(stack->pool);
  memref* ref_off = (memref*)fixed_pool_get(stack->pool,offset);
  stack->head_offset = offset;
  *ref_off = ref;
  TL("stack_push exit\n");
 }

/* void stack_push_int(refstack* stack, int value) */
/* { */
/*   memref v; */
/*   v.type = Int32; */
/*   v.data.i = value; */
/*   stack_push(stack,v); */
/* } */

/* void stack_push_double(refstack* stack, double value) */
/* { */
/*   memref v; */
/*   v.type = Double; */
/*   v.data.d = value; */
/*   stack_push(stack,v); */
/* } */
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
