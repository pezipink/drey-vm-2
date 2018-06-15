/* #include <winsock2.h> */
/* #include <windows.h> */

#include "minunit.h"
#include "global.h"
#include "memory\manager.h"
#include "datastructs\refhash.h"
#include "datastructs\refhash_test.h"
#include "datastructs\refstack.h"
#include "datastructs\refarray.h"
#include "datastructs\reflist.h"
#include "datastructs\json.h"
/* #include "vm\vm.h" */

static void setup(void)
{
  return;
}

static void teardown(void)
{
  return;
} 

MU_TEST(_dyn_resize)
{
  MemoryPool_Dynamic* pool = 0;
  dyn_pool_init(&pool,sizeof(int) * 2);
  mu_check(pool->total_size == sizeof(int) * 2 + sizeof(MemoryPool_Dynamic) - sizeof(int));
  unsigned x = dyn_pool_alloc_set(&pool, sizeof(unsigned), 42);
  //DL("X is %i\n", x);
  mu_check(x == 0);
  unsigned y = dyn_pool_alloc(&pool, sizeof(unsigned));
  unsigned z = dyn_pool_alloc_set(&pool, sizeof(unsigned) * 1024, 40);
  unsigned a = dyn_pool_alloc_set(&pool, sizeof(unsigned) * 8, 43);
  //test reallocating in the middle works
  dyn_pool_free(pool, z);
  unsigned z2 = dyn_pool_alloc_set(&pool, sizeof(unsigned) * 1024,40);
  mu_check(z == z2);
  //free again and split unsignedo two sections
  dyn_pool_free(pool, z);
  z2 = dyn_pool_alloc_set(&pool, sizeof(unsigned) * 512,41);
  mu_check(z == z2);
  //this is one unsigned less due to the header for the last block
  unsigned z3 = dyn_pool_alloc_set(&pool, sizeof(unsigned) * 511,42);
  mu_check(z3 == z2 + sizeof(unsigned) * 513);
}

MU_TEST(_dyn_resize_2)
{
  //test freeing at index 0 works
  MemoryPool_Dynamic* pool = 0;
  dyn_pool_init(&pool,sizeof(int) * 2);
  mu_check(0 == dyn_pool_alloc_set(&pool, sizeof(int), 1));
  dyn_pool_free(pool, 0);
  //tests extending the last block works
  mu_check(0 == dyn_pool_alloc_set(&pool, sizeof(int) * 2, 2));
  mu_check(12 == dyn_pool_alloc_set(&pool, sizeof(int) * 1, 3));
  mu_check(20 == dyn_pool_alloc_set(&pool, sizeof(int) * 2, 4));
  mu_check(pool->total_size == 80);
}

MU_TEST(_dyn_realloc)
{
  MemoryPool_Dynamic* pool = 0;
  dyn_pool_init(&pool,sizeof(int) * 20);
  int a = dyn_pool_alloc_set(&pool, sizeof(int), 1);
  mu_check(a==0);
  int b = dyn_pool_realloc(&pool,a,sizeof(int) * 2);
  mu_check(a==b);
  mu_check(pool->total_size == 92);

  int c = dyn_pool_realloc(&pool,a,sizeof(int) * 10);
  mu_check(b==c);
  
  int d = dyn_pool_realloc(&pool,a,sizeof(int) * 100);
  //  this should fail once extending a resized pool works
  mu_check(d==44);

  int e = dyn_pool_alloc_set(&pool,sizeof(int) * 10,0);
  //this should fail once extending a resized pool works
  mu_check(e==0);
    
}

MU_TEST(_ra_basic)
{
  
  memref ra = ra_init(sizeof(int),4);
  ra_consume_capacity(ra);
  mu_check(ra_count(ra) == 4);
  mu_check(ra_nth_int(ra,0) == 0);
  int val = 42;
  ra_set(ra,0,&val);
  mu_check(ra_nth_int(ra,0) == 42);
  int val2 = 58;
  //this will cause a realloc of the whole pool
  ra_append(ra,&val2);
  mu_check(ra_nth_int(ra,4) == 58);
  // this won't
  ra_append(ra,&val2);
  mu_check(ra_nth_int(ra,5) == 58);
  for(int i = 0; i < 10000; i++)
    {
      ra_append(ra,&val2);
    }
  mu_check(ra_count(ra) == 10006);
  int tot = 0;
  int len = ra_count(ra);
  for(int i = 0; i < len; i++)
    {
      tot += ra_nth_int(ra,i);
    }

  DL("tot %i\n",tot);
  mu_check(tot == 580158);

}

MU_TEST(_ra_split)
{
  memref ra = ra_init(sizeof(int),4);
  ra_consume_capacity(ra);
  mu_check(ra_count(ra) == 4);
  int val = 42;
  int val2 = 58;
  ra_set(ra,0,&val);
  ra_set(ra,1,&val2);
  ra_set(ra,3,&val2);
  mu_check(ra_nth_int(ra,0) == val);
  mu_check(ra_nth_int(ra,1) == val2);
  mu_check(ra_nth_int(ra,3) == val2);
  memref ra2 = ra_split_bottom(ra, 2);
  mu_check(ra_count(ra) == 2);
  mu_check(ra_count(ra2) == 2);
  mu_check(ra_nth_int(ra2,0) == val);
  mu_check(ra_nth_int(ra2,1) == val2);
  
}
MU_TEST(_ra_clone)
{
  memref ra = ra_init(sizeof(memref),4);
  memref a = int_to_memref(0);
  memref b = int_to_memref(1);
  memref c = int_to_memref(2);
  memref d = int_to_memref(3);
  
  ra_consume_capacity(ra);
  mu_check(ra_count(ra) == 4);

  ra_set_memref(ra,0,&a);
  ra_set_memref(ra,1,&b);
  ra_set_memref(ra,2,&c);
  ra_set_memref(ra,3,&d);

  memref ra2 = ra_clone(ra);
  mu_check(ra_count(ra) == 4);
  mu_check((ra_nth_memref(ra2,0)).data.i == 0);
  mu_check((ra_nth_memref(ra2,3)).data.i == 3);
  
}

MU_TEST(_ra_remove)
{
  memref ra = ra_init(sizeof(memref),4);
  memref a = int_to_memref(0);
  memref b = int_to_memref(1);
  memref c = int_to_memref(2);
  memref d = int_to_memref(3);
  
  ra_consume_capacity(ra);
  mu_check(ra_count(ra) == 4);

  ra_set_memref(ra,0,&a);
  ra_set_memref(ra,1,&b);
  ra_set_memref(ra,2,&c);
  ra_set_memref(ra,3,&d);

  ra_remove(ra,0,1);

  mu_check(ra_count(ra) == 3);
  mu_check((ra_nth_memref(ra,0)).data.i == 1);
  mu_check((ra_nth_memref(ra,1)).data.i == 2);
  mu_check((ra_nth_memref(ra,2)).data.i == 3);

  ra_remove(ra,2,1);
  mu_check(ra_count(ra) == 2);
  mu_check((ra_nth_memref(ra,0)).data.i == 1);
  mu_check((ra_nth_memref(ra,1)).data.i == 2);

  ra_remove(ra,0,2);
  mu_check(ra_count(ra) == 0);
  
}

MU_TEST(_ra_complex)
{
  /* memref ra = ra_init(4,4); */
  /* mu_check(ra_count(ra) == 4); */
  /* mu_check(ra_nth_int(ra,0) == 0); */
  /* memref ra2 = ra_init(4,4); */
  /* ra_set_memref(ra,0,&ra2); */
  /* mu_check(ra2->refcount == 1); */
  /* memref ra3 = ra_nth_memref(ra,0); */
  /* mu_check(ra3 == ra2); */
  /* ra_append_memref(&ra,&ra2); */
  /* mu_check(ra2->refcount == 2); */
  /* memref ra4 = ra_nth_memref(ra,4); */
  /* mu_check(ra3 == ra4); */
  
}

MU_TEST(_stack_clone)
{

  memref st = stack_init(1);
  memref a = malloc_int(42);
  stack_push(st,a);
  memref b = malloc_int(58);
  stack_push(st,b);

  memref c = stack_clone(st);

  mu_check(stack_peek(c).data.i == 58);

  stack_pop(c);

  mu_check(stack_peek(c).data.i == 42);
  mu_check(stack_peek(st).data.i == 58);

  
  
}
MU_TEST(_list_basic)
{
  memref lr1 = rl_init(); //empty list

  mu_check(lr1.type == List);
  list* l1 = deref(&lr1);
  mu_check(l1->head.type == 0);
  mu_check(l1->tail.type == 0);
  
  memref lr2 = rl_init_h_t(int_to_memref(42), lr1);
  mu_check(lr2.type == List);

  list* l2 = deref(&lr2);

  mu_check(l2->head.type == Int32 && l2->head.data.i == 42);
  mu_check(l2->tail.type == List && l2->tail.data.i == lr1.data.i);
  
  
  memref lr3 = rl_init_h_t(int_to_memref(58), lr2);

  list* l3 = deref(&lr3);

  mu_check(l3->head.type == Int32 && l3->head.data.i == 58);
  mu_check(l3->tail.type == List && l3->tail.data.i == lr2.data.i);
  

}
MU_TEST(_stack_basic)
{
    memref st = stack_init(1);
    memref st2 = stack_init(1);

    refstack* rs1 = deref(&st);
    refstack* rs2 = deref(&st2);
    
    rs1->pool->owner = &rs1->pool;
    rs2->pool->owner = &rs2->pool;

    for(int i = 0; i < 1000; i++)
      {
        /* printf("%i\n", i); */
        /* printf("A\n"); */
        stack_push(st, int_to_memref(i));
        /* printf("B\n"); */
        stack_push(st2, int_to_memref(i));
      }
}

MU_TEST(_json_main)
{
  char* str = " { \"test-key\" :  [  \"hello world\" , \"test\" ], \
                  \"key-2\" : [1, 2, 3   , 4  , 5, true, false] }";

  memref o = json_to_object(&str, strlen(str) - 1);
  print_hash(o, 0);

  memref s = object_to_json(o);

  ra_wl(s);
  
  //  ra_wl(o);
}


MU_TEST(_vm_a)
{
  /* vm v = init_vm();   */
  /* run(&v);   */

}

/* MU_TEST(_gc_main) */
/* { */
/*   gc_print_stats(); */
/*   memref st = hash_init(0); */
/*   memref sa = ra_init_str("A"); */
/*   hash_set(st,int_to_memref(1),sa); */
/*   for(int i = 0; i < 100; i ++) */
/*     { */
/*       ra_init_str("B"); */
/*     } */
/*   gc_print_stats(); */
/*   int off = 0; */
/*   for(int i = 0; i < 50; i++) */
/*     { */
/*       off = gc_clean_step(off); */
/*       gc_print_stats(); */
/*     } */
    
  
/* } */

enum cell_type
  {
    REF = 0,
    STR = 1,
    FUN = 2
  };

typedef struct cell
{
  enum cell_type type;
  union
  {
    struct cell* c;
    int s;
  } data;
} cell;
enum scolog_op
  {
    put_struct = 0,
    set_var = 1,
    set_val = 2,
    get_struct = 3,
    unify_var = 4,
    unify_val = 5
  };

cell* deref_cell(cell* start)
{
  if(start->type == REF && start->data.c != start)
    {
      return deref_cell(start->data.c);
    }
  else
    {
      return start;
    }
}

void push(memref ra, void* val)
{
  ra_append(ra, val);
}


void* pop(memref ra)
{
  void* res  = ra_nth(ra, ra_count(ra)-1);
  ra_dec_count(ra);
  return res;
}

void unify(cell* a1, cell* a2, memref arity)
{
  int fail = 0;
  memref pdl = ra_init(sizeof(cell),128);
  cell* d1, *d2;
  cell* t1, *t2;
  while(ra_count(pdl) > 0 && !fail)
    {
      d1 = deref_cell(pop(pdl));
      d2 = deref_cell(pop(pdl));
      if(d1 != d2)
        {
          if(d1->type == REF || d2->type == REF)
            {
              d1->data.c = d2->data.c;
            }
          else
            {
              t1 = d1->data.c;
              t2 = d2->data.c;
              if(t1->data.s == t2->data.s)
                {
                  int a = hash_get(arity, int_to_memref(t1->data.s)).data.i;
                  t1++;
                  t2++;
                  for(int i = 0; i < a; i++)
                    {
                      push(pdl, t1 + i);
                      push(pdl, t2 + i);
                    }
                }
              else
                {
                  fail = 1;
                }
            }
          
        }

    }
}

MU_TEST(_scolog_a)
{
  memref s = hash_init(7);
  memref arity = hash_init(7);
  hash_set(s, int_to_memref(0), ra_init_str("f/1"));
  hash_set(arity, int_to_memref(0), int_to_memref(1));

  hash_set(s, int_to_memref(1), ra_init_str("h/2"));
  hash_set(arity, int_to_memref(0), int_to_memref(2));
    
  hash_set(s, int_to_memref(2), ra_init_str("p/3"));
  hash_set(arity, int_to_memref(0), int_to_memref(3));
  memref heap = ra_init(sizeof(cell),128);
  ra_consume_capacity(heap);
  cell* reg[6] = {0,0,0,0,0,0};
  int prog[21] =
    {0, 0, 4, //put f/1 x4
     1, 5,    // set x5
     0, 1, 3, // put h/2 x3
     1, 2,    // set x2
     2, 5,    // set x5
     0, 2, 3, // put p/3 x1
     2, 2,    // set x2
     2, 3,    // set x3
     2, 4    // set x4
    };

  cell* h = ra_nth(heap,0);
  cell* c;
  cell* S;
  int mode = 0;
  for(int i = 0; i < 21; i++)
    {
      switch(prog[i])
        {
        case put_struct:
          h->type = STR;
          h->data.c = h + 1;
          h++;
          h->type = FUN;
          h->data.s = prog[++i];
          reg[prog[++i]] = h;
          h++;
          break;

        case set_var:
          h->type = REF;
          h->data.c = h;
          reg[prog[++i]] = h;
          h++;
          break;

        case set_val:
          *h = *reg[prog[++i]];
          h++;
          break;

        case get_struct:
          int str = prog[++i];
          c = deref_cell(reg[prog[++i]]);
          if(c->type == REF)
            {
              h->type = STR;
              h->data.c = h + 1;
              c->data.c = h;
              h++;
              h->type = FUN;
              h->data.s = str;
              h++;
              mode=1;
            }
          else if(c->type == STR)
            {
              if((c++)->data.s == str)
                {
                  *S = *c->data.c;
                  mode = 0;
                }
              else
                {
                  printf("FAILURE b");
                }
            }
          else
            {
              printf("FAILURE c");
              return;
            }
          break;

        case unify_var:
          if(mode == 0) // read
            {
              *reg[prog[++i]] = *S;
            }
          else
            {
              h->type = REF;
              h->data.c = h;
              reg[prog[++i]] = h;
              h++;
              
            }
          S++;
          break;

        case unify_val:
          if(mode == 0)
            {
              unify(reg[prog[++i]],S,arity);
            }
          else
            {
              *h = *reg[prog[++i]];
              h++;
            }
          S++;
          break;
          
        }
    }


  
  h = ra_nth(heap,0);
  int index = 0;
  while(index < 12)
    {
      printf("address %p index 1 type %i\n", h, h->type);
      if(h->type == FUN)
        printf("string index %i\n", h->data.s);
      else
        printf("cell %p\n", h->data.c);
      index++;
      h++;
    }
}


MU_TEST_SUITE(gc_suite)
{
  MU_SUITE_CONFIGURE(&setup,&teardown);

  //  fixed_pool_init(&int_memory,sizeof(int),100);
  fixed_pool_init(&ref_memory,sizeof(ref),1); 
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  fixed_pool_init(&scope_memory,sizeof(scope),10);
  fixed_pool_init(&stack_memory,sizeof(refstack),1);
  fixed_pool_init(&kvp_memory,sizeof(key_value),10);
  fixed_pool_init(&go_memory,sizeof(gameobject),10);
  fixed_pool_init(&loc_memory,sizeof(location),10);
  fixed_pool_init(&loc_ref_memory,sizeof(locationref),10);
  
  dyn_pool_init(&dyn_memory,sizeof(int) * 10);
  MU_RUN_TEST(_gc_main);
}

MU_TEST_SUITE(json_suite)
{
  MU_SUITE_CONFIGURE(&setup,&teardown);

  //  fixed_pool_init(&int_memory,sizeof(int),100);
  fixed_pool_init(&ref_memory,sizeof(ref),1); 
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  fixed_pool_init(&scope_memory,sizeof(scope),10);
  fixed_pool_init(&stack_memory,sizeof(refstack),1);
  fixed_pool_init(&kvp_memory,sizeof(key_value),10);
  fixed_pool_init(&go_memory,sizeof(gameobject),10);
  fixed_pool_init(&loc_memory,sizeof(location),10);
  fixed_pool_init(&loc_ref_memory,sizeof(locationref),10);
  
  dyn_pool_init(&dyn_memory,sizeof(int) * 10);
  MU_RUN_TEST(_json_main);
}

MU_TEST_SUITE(test_suite)
{
  MU_SUITE_CONFIGURE(&setup,&teardown);

  //  fixed_pool_init(&int_memory,sizeof(int),100);
  fixed_pool_init(&ref_memory,sizeof(ref),1); //10mb of refs
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  fixed_pool_init(&scope_memory,sizeof(scope),1);
  fixed_pool_init(&func_memory,sizeof(function),10);

  fixed_pool_init(&stack_memory,sizeof(refstack),1);
  fixed_pool_init(&kvp_memory,sizeof(key_value),1000000);
  dyn_pool_init(&dyn_memory,sizeof(int) * 1);
  fixed_pool_init(&go_memory,sizeof(gameobject),10);
  fixed_pool_init(&loc_memory,sizeof(location),10);
  fixed_pool_init(&loc_ref_memory,sizeof(locationref),10);
  fixed_pool_init(&list_memory,sizeof(list),10);

  /* MU_RUN_TEST(_dyn_resize); */
  /* MU_RUN_TEST(_dyn_resize_2); */
  
  /* MU_RUN_TEST(_dyn_realloc); */
  /* MU_RUN_TEST(_ra_basic); */
  /* MU_RUN_TEST(_ra_remove); */
  /* MU_RUN_TEST(_ra_clone); */
  /* MU_RUN_TEST(_ra_split); */
  /* MU_RUN_TEST(_ra_complex); */
  /* MU_RUN_TEST(_stack_clone); */
  MU_RUN_TEST(_list_basic);
  MU_RUN_TEST(_scolog_a);
    /* MU_RUN_TEST(_stack_basic); */
  /* MU_RUN_TEST(_stack_complex); */
  DL("core tests finihsed\n");
 
  //     MU_RUN_TEST(_vm_a);
}



int main(int argc, char *argv[])
{
  /* Mu_RUN_SUITE(gc_suite); */

  /* MU_RUN_SUITE(test_suite); */
  MU_RUN_SUITE(json_suite);
  /* MU_RUN_SUITE(refhash_suite); */

  /* gc_clean_full(); */
  
  MU_REPORT();
  return 0;
}
