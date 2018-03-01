#include "minunit.h"
#include "global.h"
#include "memory\manager.h"
#include "datastructs\refhash.h"
#include "datastructs\refhash_test.h"
#include "datastructs\refstack.h"
#include "datastructs\refarray.h"
#include "vm\vm.h"

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

MU_TEST(_ra_remove)
{
  memref ra = ra_init(sizeof(int),4);
  ra_consume_capacity(ra);
  mu_check(ra_count(ra) == 4);
  int val = 42;
  int val2 = 58;
  ra_set(ra,0,&val);
  ra_set(ra,1,&val2);
  mu_check(ra_nth_int(ra,0) == val);
  mu_check(ra_nth_int(ra,1) == val2);
  ra_remove(ra,0,1);
  mu_check(ra_count(ra) == 3);
  mu_check(ra_nth_int(ra,0) == val2);
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

MU_TEST(_stack_basic)
{
  //the stack only ever holds memref values

  memref st = stack_init(1);
  memref a = malloc_int(42);
  stack_push(st,a);
  memref b = stack_peek(st);
  mu_check(a.data.i == b.data.i);
  memref c = stack_pop(st);

  for(int i = 0; i < 1000; i ++)
    {
      stack_push(st,a);
    }
  
  for(int i = 0; i < 1000; i ++)
    {
      stack_pop(st);
    } 
}


MU_TEST(_stack_complex)
{
  //stack and hash interaction
  /* refstack st = stack_init(1); */
  /* memref h = int_to_memref(1); */
  /* stack_push(&st, h); */
  
}

MU_TEST(_vm_a)
{
  vm v = init_vm();  
  run(&v);  

}

MU_TEST(_gc_main)
{
  gc_print_stats();
  memref st = hash_init(0);
  memref sa = ra_init_str("A");
  hash_set(st,int_to_memref(1),sa);
  for(int i = 0; i < 100; i ++)
    {
      ra_init_str("B");
    }
  gc_print_stats();
  int off = 0;
  for(int i = 0; i < 50; i++)
    {
      off = gc_clean_step(off);
      gc_print_stats();
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

MU_TEST_SUITE(test_suite)
{
  MU_SUITE_CONFIGURE(&setup,&teardown);

  //  fixed_pool_init(&int_memory,sizeof(int),100);
  fixed_pool_init(&ref_memory,sizeof(ref),1); //10mb of refs
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  fixed_pool_init(&scope_memory,sizeof(scope),10);
  fixed_pool_init(&func_memory,sizeof(function),10);

  fixed_pool_init(&stack_memory,sizeof(refstack),1);
  fixed_pool_init(&kvp_memory,sizeof(key_value),1000000);
  dyn_pool_init(&dyn_memory,sizeof(int) * 1);
  fixed_pool_init(&go_memory,sizeof(gameobject),10);
  fixed_pool_init(&loc_memory,sizeof(location),10);
  fixed_pool_init(&loc_ref_memory,sizeof(locationref),10);

  /* MU_RUN_TEST(_dyn_resize); */
  /* MU_RUN_TEST(_dyn_resize_2); */
  
  /* MU_RUN_TEST(_dyn_realloc); */
  /* MU_RUN_TEST(_ra_basic); */
  /* MU_RUN_TEST(_ra_remove); */
  MU_RUN_TEST(_ra_split);
  /* MU_RUN_TEST(_ra_complex); */
  /* MU_RUN_TEST(_stack_basic); */
  /* MU_RUN_TEST(_stack_complex); */
  DL("core tests finihsed\n");
 
     MU_RUN_TEST(_vm_a);
}



int main(int argc, char *argv[])
{
  /* MU_RUN_SUITE(gc_suite); */

  MU_RUN_SUITE(test_suite);
  /* MU_RUN_SUITE(refhash_suite); */

  /* gc_clean_full(); */
  
  MU_REPORT();
  return 0;
}
