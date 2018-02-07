#include "minunit.h"
#include "global.h"
#include "memory\manager.h"
#include "datastructs\refhash.h"
#include "datastructs\refstack.h"


static void setup()
{
  fixed_pool_init(&int_memory,sizeof(int),1024);
  fixed_pool_init(&ref_memory,sizeof(memref),1024);
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  dyn_pool_init(&dyn_memory,sizeof(int) * 1024);
}

static void teardown()
{
  
} 


// REFHASH TESTS

// INT KEY AND VALUE
MU_TEST(_hash_int_key_and_value)
{
  memref* h = hash_init(0);
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
  memref* h = hash_init(0);
  mu_check(h != NULL);
  int key = 42;
  int value = 58;
  memref* i = malloc_int(42);
  memref* j = malloc_int(58);
  hash_set(h,i,j);
  mu_check(hash_contains(h,i));
  mu_check(!hash_contains(h,j));
}


MU_TEST(_dyn_resize)
{
  MemoryPool_Dynamic* pool = 0;
  dyn_pool_init(&pool,sizeof(int) * 2);
  mu_check(pool->total_size == sizeof(int) * 2 + sizeof(MemoryPool_Dynamic) - sizeof(int));
  int x = dyn_pool_alloc_set(&pool, sizeof(int), 42);
  mu_check(x == 0);
  int y = dyn_pool_alloc(&pool, sizeof(int));
  int z = dyn_pool_alloc_set(&pool, sizeof(int) * 1024, 40);
  int a = dyn_pool_alloc_set(&pool, sizeof(int) * 8, 43);
  //test reallocating in the middle works
  dyn_pool_free(pool, z);
  int z2 = dyn_pool_alloc_set(&pool, sizeof(int) * 1024,40);
  mu_check(z == z2);
  //free again and split into two sections
  dyn_pool_free(pool, z);
  z2 = dyn_pool_alloc_set(&pool, sizeof(int) * 512,41);
  mu_check(z == z2);
  //this is one int less due to the header for the last block
  int z3 = dyn_pool_alloc_set(&pool, sizeof(int) * 511,42);
  mu_check(z3 == z2 + sizeof(int) * 513);
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

MU_TEST_SUITE(test_suite)
{
  MU_SUITE_CONFIGURE(&setup,&teardown);
  MU_RUN_TEST(_hash_int_key_and_value);
  MU_RUN_TEST(_hash_contains_int);
  MU_RUN_TEST(_dyn_resize);
  MU_RUN_TEST(_dyn_resize2);  
}

int main(int argc, char *argv[])
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return 0;
}
