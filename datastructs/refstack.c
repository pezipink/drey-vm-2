#include "refstack.h"
#include "..\memory\manager.h"
#include "..\global.h"

refstack stack_init(int initialSize)
{
  TL("stack_init entry\n");
  refstack stack;
  stack.head_offset = 0;
  fixed_pool_init(&stack.pool, sizeof(int), initialSize);
  TL("stack_init exit\n");
  return stack;
}

int stack_peek(refstack* stack)
{
  return *(int*)fixed_pool_get(stack->pool,stack->head_offset);
}

memref* stack_pop(refstack* stack)
{
  //  assert(stack->head_offset > 0);
  int* ref_off = (int*)fixed_pool_get(stack->pool,stack->head_offset);
  DL("popping stack with ref offset of %i",*ref_off);
  memref* ref = (memref*)fixed_pool_get(ref_memory,*ref_off);
  ref->refcount--;
  fixed_pool_free(stack->pool,stack->head_offset);
  stack->head_offset -= sizeof(int);
  return ref;
}

void stack_push(refstack* stack, memref* ref)//should this be a ptr??
{
  DL("stack_push enter\n");
  ref->refcount++;
  int offset = fixed_pool_alloc(stack->pool);
  DL("new offset is %i\n",offset);
  int* ref_off = (int*)fixed_pool_get(stack->pool,offset);
  stack->head_offset = offset;
  *ref_off = ref->ref_off;
}

void stack_push_int(refstack* stack, int value)
{
  stack_push(stack,malloc_int(42));
}


void debug_print_stack(refstack* stack)
{
  int* ptr = (int*)fixed_pool_get(stack->pool,0);
  DL("stack head is at offset %i\n", stack->head_offset);
  int i = 0;
  do
    {
      DL("index %i ref offset %i\n",i,*ptr);
      memref* ref = (memref*)fixed_pool_get(ref_memory,*ptr);
      DL("\tmemref data:\n");
      DL("\t\ttype %i\n",ref->type);
      DL("\t\trefcount %i\n",ref->refcount);
      DL("\t\tref_off %i\n",ref->ref_off);
      DL("\t\ttarg_off %i\n",ref->targ_off);
      i += sizeof(int);
      ptr++;
    }
  while(i <= stack->head_offset);
}
