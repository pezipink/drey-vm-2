#include "gc.h"
#include "manager.h"
#include "..\global.h"
#include "..\datastructs\refhash.h"
#include "..\datastructs\refarray.h"
#include "..\datastructs\refstack.h"
#include "..\datastructs\reflist.h"
#include "..\vm\vm.h"
#include <assert.h>
#include <stdio.h>

void gc_print_stats()
{
  printf("2 reference pool at %p\n",ref_memory);
  int fcounts[50];
  int ucounts[50];
  for(int i = 0; i < 50; i++)
    {
      fcounts[i]=0;
      ucounts[i]=0;
    }
  int max = ref_memory->element_count;
  int toFree = 0;
  int inUse = 0;
  int unused = 0;
  ref* item = (ref*)&ref_memory->data;
  int offset = 0;
  for(int i = 0; i < max; i++)
    {
      if(item->type < 0 || item->type > LocationReference)
        {
          DL("!!! UNKNONW TYPE %i\n",item->type);
        }
      else
        {
          /* if(item->type > 0) */
          /*   { */
          /*     if(item->refcount == 0) */
          /*       { */
                       ucounts[item->type]++;
              /*   } */
              /* else */
              /*   { */
              /*     ucounts[item->type]++; */
              /*   } */
            /* } */
          /* else */
          /*   { */
          /*     unused++; */
          /*   } */
        }
      item++;
      offset+=sizeof(ref);
    }
  DL("GC STATS:\n");
  DL("Hash\t\t to free %i\tin use %i\n",fcounts[Hash],ucounts[Hash]);
  DL("Array\t\t to free %i\tin use %i\n",fcounts[Array],ucounts[Array]);
  DL("String\t\t to free %i\tin use %i\n",fcounts[String],ucounts[String]);
  DL("KVP\t\t to free %i\tin use %i\n",fcounts[KVP],ucounts[KVP]);
  DL("Stack\t\t to free %i\tin use %i\n",fcounts[Stack],ucounts[Stack]);
  DL("Scope\t\t to free %i\tin use %i\n",fcounts[Scope],ucounts[Scope]);
   DL("Function\t to free %i\tin use %i\n",fcounts[Function],ucounts[Function]);
      DL("List node\t to free %i\tin use %i\n",fcounts[List],ucounts[List]);

 }


//first version uses recursion
void scan_graph(memref  current, bool action (memref ))
{
  if(!action(current))
    {
      return;
    }
  switch(current.type)
    {
    case Hash:
      TL("scanning hash\n");
        refhash* hash = deref(&current);
        action(hash->buckets);
        int kvp_count = hash->kvp_count;
        if(kvp_count == 0)
          {
            break;
          }
        int bucketCount = ra_count(hash->buckets);
        TL("bucket count %i\n", bucketCount);
        key_value* kvp = 0;
        int eleCount = 0;
        
        for(int i = 0; i < bucketCount; i++)
          {
            TL("scanning bucket %i\n", i); 
            memref r = ra_nth_memref(hash->buckets, i);
            if(r.type == KVP)
              {
                while(true)
                  {
                    eleCount++;
                    scan_graph(r, action);
                    kvp = deref(&r);                  
                    if(kvp->next.type == KVP)
                      {
                        r = kvp->next;
                      }
                    else
                      {
                        break;
                      }
                  }
              }
            if(eleCount == kvp_count)
              {
                break;
              }
          }
        TL("finished scanning hash\n");
        break;
    case KVP:
      //maybe the kvp traverse should go here
      kvp = deref(&current);
      scan_graph(kvp->key,action);
      scan_graph(kvp->val,action);
      break;
    case GameObject:
      gameobject* go = deref(&current);
      scan_graph(go->visibility,action);
      scan_graph(go->props,action);
      scan_graph(go->location_key,action);
      break;
    case Location:
      location* loc = deref(&current);
      scan_graph(loc->siblings,action);
      scan_graph(loc->children,action);
      scan_graph(loc->parent,action);
      scan_graph(loc->key,action);
      scan_graph(loc->props,action);
      scan_graph(loc->objects,action);
      break;
    case LocationReference:
      locationref* lref = deref(&current);
      scan_graph(lref->target_key,action);
      scan_graph(lref->props,action);
      break;
    case Scope:
      TL("scanning scope\n");
      scope* s = deref(&current);
      scan_graph(s->locals, action);
      if(s->closure_scope.type == Scope)
        {
          scan_graph(s->closure_scope,action);
        }
      TL("end scanning scope\n");
      break;
    case Array:
      TL("scanning array\n");
      refarray* ra = deref(&current);
      int max = ra->element_count;
      TL("array has %i elements\n",max);
      memref* r = (memref*)&ra->data;
      for(int i = 0; i < max; i++)
        {
          TL("index %i has type %i\n", i, r->type);
          scan_graph(*r, action);
          r++;
        }
      TL("end scanning array\n");
      break;

    case List:
      TL("scanning list\n");
      list* lst = deref(&current);
      scan_graph(lst->head, action);
      scan_graph(lst->tail, action);
      TL("end scanning list\n");
      break;

    case Function:
      TL("Scanning function..\n");
      function* f = deref(&current);
      scan_graph(f->closure_scope, action);
      TL("end scanning fucntion\n");
      break;
        
    case Stack:
      TL("scanning stack\n");
      refstack* rs = deref(&current);
      int offset = 0;      
      if(rs->head_offset >= 0)
        {
          memref* r2 = fixed_pool_get(rs->pool,0);
          scan_graph(*r2,action);
          while(offset != (int)rs->head_offset)
            {
              r2++;
              offset += sizeof(memref);
              TL("stack offset %i\n", offset);
              scan_graph(*r2, action);
            }
        }
      TL("end scanning stack\n");
      break;
              

    }
}


bool gc_marker(memref mref)
{
  if(!is_value(mref))
    {
      ref* r = get_ref(mref.data.i);
      if(r->flags == 1)
        {
          return 0;
        }
      r->flags = 1;
      return 1;
    }
  return 0;
}

void gc_mark_n_sweep(vm* vm)
{
  TL("GC marking...\n");
  int max_fiber = ra_count(vm->fibers);
  for(int fi = 0; fi < max_fiber; fi++)
    {
      fiber* f = (fiber*)ra_nth(vm->fibers,fi);

      scan_graph(f->valid_responses, gc_marker);
      gc_marker(f->waiting_client);
      gc_marker(f->waiting_data);
      gc_marker(f->exec_contexts);
          
      int max_ec = ra_count(f->exec_contexts);
      for(int ei = 0; ei < max_ec; ei++)
        {              
          exec_context* ec = (exec_context*)ra_nth(f->exec_contexts,ei);
          scan_graph(ec->eval_stack, gc_marker);
          scan_graph(ec->scopes, gc_marker);
              
        }          
    }



  gc_marker(vm->fibers);
  gc_marker(vm->program);
  
  scan_graph(vm->string_table, gc_marker);
  scan_graph(vm->u_objs,gc_marker);
  //  scan_graph(vm->u_locrefs,gc_marker);
  scan_graph(vm->u_locs,gc_marker);
  //for now the roots are just the stack.

  TL("GC Sweeping...\n");
  int offset = 0;
  int max = ref_memory->element_count ;
  int toFree = 0;
  int inUse = 0;
  int unused = 0;
  ref* item = (ref*)&ref_memory->data;
  for(int i = 0; i < max; i++)
    {
      if(item->type != 0)
        {
          if(item->flags == 0)
            {
              TL("freeing item tpye %i!\n",item->type);
              free_ref(item,offset);
            }
          else
            {
              //              DL("item survives!\n");
              item->flags = 0;
            }
        }
      item++;
      offset+=sizeof(ref);

    }
}


/* int gc_clean_full() */
/* { */
/*   int cycles = 1; */
/*   //  DL("iteration 0\n"); */
/*   while(gc_clean_sweep()) */
/*     { */
/*       //      DL("iteration %i\n",cycles); */
/*       cycles ++; */
/*     } */
/*   return cycles; */
/* } */



void gc_clean_cycles()
{
  // cycle detection
}
