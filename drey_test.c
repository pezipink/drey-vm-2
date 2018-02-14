#include "minunit.h"
#include "global.h"
#include "memory\manager.h"
#include "datastructs\refhash.h"
#include "datastructs\refstack.h"
#include "datastructs\refarray.h"


static void setup(void)
{
  fixed_pool_init(&int_memory,sizeof(int),1);
  fixed_pool_init(&ref_memory,sizeof(memref),1428470); //10mb of refs
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  fixed_pool_init(&kvp_memory,sizeof(key_value),1);
  dyn_pool_init(&dyn_memory,sizeof(int) * 1024);
  return;
}

static void teardown(void)
{
  return;
} 


// REFHASH TESTS
MU_TEST(_kvp_pool)
{
  memref* a = malloc_int(10);
  memref* b = malloc_int(20);
  memref* bb = malloc_int(20);
  memref* c = malloc_kvp(a,b,(memref*)0);
  memref* d = malloc_kvp(a,b,(memref*)0);
  memref* e = malloc_kvp(a,b,(memref*)0);
  memref* f = malloc_kvp(a,b,(memref*)0);

}
// INT KEY AND VALUE
MU_TEST(_hash_int_key_and_value)
{
  memref* h = hash_init(1);
  mu_check(h != NULL);
  int key = 42;
  int value = 58;
  memref* i = malloc_int(42);
  memref* j = malloc_int(58);
  hash_set(h,i,j);
  memref* r = hash_get(h, i);
  int* x = (int*)deref(r);
  mu_check(*x == value);
}

MU_TEST(_hash_contains_int)
{
  memref* h = hash_init(1);
  mu_check(h != NULL);
  int key = 42;
  int value = 58;
  memref* i = malloc_int(42);
  memref* j = malloc_int(58);
  hash_set(h,i,j);
  mu_check(hash_contains(h,i));
  mu_check(!hash_contains(h,j));
}

MU_TEST(_hash_resize)
{

  memref* h = hash_init(1);
  mu_check(h != NULL);
  int key = 42;
  int value = 58;
    memref* i = malloc_int(42);
  memref* j = malloc_int(58);
  memref* k = malloc_int(1834);
  memref* l = malloc_int(22);
  memref* m = malloc_int(-390656);
    
  /* hash_set(h,i,j); */
  /* hash_set(h,j,j); */
  /* hash_set(h,k,j); */
  /* hash_set(h,l,j); */
  /* hash_set(h,m,j); */

  for(int z = 0; z < 100000; z++)
    {
      hash_set(h,malloc_int(z),j);
    }

  DL("FINISHED\n");
  refhash* hash = deref(h);

  int collisions = 0;
  int unusedbuckets = 0;
  int maxcoll = 0;
  for(int i = 0; i < ra_count(hash->buckets); i++)
    {
      memref* c = ra_nth_memref(hash->buckets, i);
      if(c == NULL)
        {
          unusedbuckets++;
        }
      else
        {
          key_value* kvp = deref(c);
          if(kvp->next != NULL)
            {
              collisions ++;
      
              int count = 0;
              while(kvp->next != NULL)
                {
                  count++;
                  kvp = deref(kvp->next);
                }
              if(count > maxcoll)
                {
                  maxcoll = count;
                }
            }
              
          
        }

    }
  DL("hash ended with %i kvps and %i buckets\n", hash->kvp_count, ra_count(hash->buckets));
  DL("hash has %i unused buckets and at least %i collisions\n", unusedbuckets, collisions);
  DL("max items in a bucket %i\n", maxcoll);
  
    /* for(int z = 0; z < 1000; z++) */
    /* { */
    /*   memref* zz = malloc_int(z); */
    /*   memref* val = hash_get(h,zz); */
    /*   int xx = *(int*)deref(val); */
    /*   //      DL("index % i has val %i\n", z, xx); */
    /* } */

}

MU_TEST(_dyn_resize)
{
  MemoryPool_Dynamic* pool = 0;
  dyn_pool_init(&pool,sizeof(int) * 2);
  mu_check(pool->total_size == sizeof(int) * 2 + sizeof(MemoryPool_Dynamic) - sizeof(int));
  unsigned x = dyn_pool_alloc_set(&pool, sizeof(unsigned), 42);
  DL("X is %i\n", x);
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
    //this should fail once extending a resized pool works

  mu_check(d==76);

  int e = dyn_pool_alloc_set(&pool,sizeof(int) * 10,0);
  //this should fail once extending a resized pool works
  mu_check(e==0);
    
}

MU_TEST(_ra_basic)
{
  memref* ra = ra_init(4,4); 
  mu_check(ra_count(ra) == 4);
  mu_check(ra_nth_int(ra,0) == 0);
  int val = 42;
  ra_set(ra,0,&val);
  mu_check(ra_nth_int(ra,0) == 42);
  int val2 = 58;
  //this will cause a realloc of the whole pool
  ra_append(&ra,&val2);
  mu_check(ra_nth_int(ra,4) == 58);
  // this won't
  ra_append(&ra,&val2);
  mu_check(ra_nth_int(ra,5) == 58);
  memref* ra2 = ra_init(4,4);
  
}

MU_TEST(_ra_complex)
{
  memref* ra = ra_init(4,4); 
  mu_check(ra_count(ra) == 4);
  mu_check(ra_nth_int(ra,0) == 0);
  memref* ra2 = ra_init(4,4); 
  ra_set_memref(ra,0,&ra2);
  mu_check(ra2->refcount == 1);
  memref* ra3 = ra_nth_memref(ra,0);  
  mu_check(ra3 == ra2);
  ra_append_memref(&ra,&ra2);
  mu_check(ra2->refcount == 2);
  memref* ra4 = ra_nth_memref(ra,4);  
  mu_check(ra3 == ra4);
  
} 

MU_TEST(_stack_basic)
{
  //the stack only ever holds memref pointers on it
  refstack st = stack_init(1);
  memref* a = malloc_int(42);
  stack_push(&st,a);
  memref* b = stack_peek(&st);
  mu_check(a->targ_off==b->targ_off);
  mu_check(a==b);
  mu_check(a->refcount == 1);
  stack_push(&st,a);
  mu_check(b->refcount==2);
  mu_check(a->refcount==2);
  memref* c = stack_pop(&st);
  mu_check(b==c);
  mu_check(a==c);
  mu_check(c->refcount == 1);

}

MU_TEST(_stack_complex)
{
  //stack and hash interaction
  refstack st = stack_init(1);
  memref* h = hash_init(1);
  stack_push(&st, h);
  memref* i = malloc_int(42);
  memref* j = malloc_int(58);
  mu_check(i->refcount == 0);
  mu_check(j->refcount == 0);  
  hash_set(stack_peek(&st),i,j);
  mu_check(i->refcount == 1);
  mu_check(j->refcount == 1);
  mu_check(hash_count(h) == 1);  
  stack_push(&st, i);
  mu_check(i->refcount == 2);
  hash_remove(h,i);
  mu_check(hash_count(h) == 0);
  mu_check(!hash_contains(h,i));
  mu_check(i->refcount == 1);
  
  
}


MU_TEST_SUITE(test_suite)
{
  MU_SUITE_CONFIGURE(&setup,&teardown);
  MU_RUN_TEST(_hash_int_key_and_value);
  MU_RUN_TEST(_hash_contains_int);
  MU_RUN_TEST(_hash_resize);
  MU_RUN_TEST(_kvp_pool);
  MU_RUN_TEST(_dyn_resize);
  MU_RUN_TEST(_dyn_resize_2);
  MU_RUN_TEST(_dyn_realloc);
  MU_RUN_TEST(_ra_basic);
  MU_RUN_TEST(_ra_complex);
  MU_RUN_TEST(_stack_basic);
  MU_RUN_TEST(_stack_complex);

}

int main(int argc, char *argv[])
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return 0;
}
