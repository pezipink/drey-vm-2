#include "reflist.h"

#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"



memref rl_init()
{
  TL("rl_init\n");
  int lst_off = fixed_pool_alloc(list_memory);
  list* lst = (list*)fixed_pool_get(list_memory, lst_off);
  lst->head = nullref();
  lst->tail = nullref();
  memref ref = malloc_ref(List, lst_off);
  return ref;
}

memref rl_init_h_t(memref head, memref tail)
{
  TL("rl_init_h_t\n");
  int lst_off = fixed_pool_alloc(list_memory);
  list* lst = (list*)fixed_pool_get(list_memory, lst_off);
  lst->head = head;
  lst->tail = tail;
  memref ref = malloc_ref(List, lst_off);
  return ref;
}

memref rl_head(memref listref)
{
  list* lst = deref(&listref);
  return lst->head;
}

memref rl_tail(memref listref)
{
  list* lst = deref(&listref);
  return lst->tail;
}
/* void rl_walk(memref head, bool action (memref)) */
/* { */
/*   list* h = deref(&head); */
/*   if(h->head.type == 0) */
/*     { */
/*       return; */
/*     } */
  
/*   if(!action(h->head)) */
/*     { */
/*       rl_walk(h->tail, action); */
/*     }   */
/* } */


