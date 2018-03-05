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

memref* stack_peek_ref(memref stack_ref)
{
  refstack* stack = deref(&stack_ref);
  return (memref*)fixed_pool_get(stack->pool,stack->head_offset);
}

memref stack_pop(memref stack_ref)
{
  TL("stack_pop entry\n");
  refstack* stack = deref(&stack_ref);
  TL("offset currently %i\n", stack->head_offset);
  //  assert(stack->head_offset > 0);
  #if DEBUG
  if(stack->head_offset < 0x0)
    {
      DL("!!!attempted to pop stack with no values !!!!\n");
    }
  #endif
  memref ref = *(memref*)fixed_pool_get(stack->pool,stack->head_offset);
  //TL("popping stack with ref offset of %i",*ref);  
  //memref* ref = (memref*)fixed_pool_get(ref_memory,*ref_off);
  fixed_pool_free(stack->pool,stack->head_offset);
  stack->head_offset -= sizeof(memref);
  TL("offset now %i\n", stack->head_offset);
  TL("stack_pop exit\n");
  return ref;
}

void stack_push(memref stack_ref, memref ref)
{
  TL("stack_push enter\n");
  refstack* stack = deref(&stack_ref);
  TL("pushing to stack at %p with pool %p\n", stack, stack->pool);
  int offset = fixed_pool_alloc(stack->pool);
  memref* ref_off = (memref*)fixed_pool_get(stack->pool,offset);
  TL("pushed to offset %i\n", offset);
  stack->head_offset = offset;
  *ref_off = ref;
  TL("stack_push exit\n");
 }

int stack_head_offset(memref stack_ref)
{
 refstack* stack = deref(&stack_ref);
 return stack->head_offset;
}


memref stack_clone(memref stack_ref)
{ 
  int off = fixed_pool_alloc(stack_memory);
  refstack* new_stack = (refstack*)fixed_pool_get(stack_memory,off);
  refstack* old_stack = deref(&stack_ref);

  new_stack->head_offset = old_stack->head_offset;  

  fixed_pool_init(&new_stack->pool, sizeof(memref), old_stack->pool->element_count);

  int size =
    old_stack->pool->element_size * old_stack->pool->element_count;

  new_stack->pool->free_offset = old_stack->pool->free_offset;

  memcpy(&new_stack->pool->data, &old_stack->pool->data, size);
  
  memref ref = malloc_ref(Stack,off);
  return ref;
  
}
