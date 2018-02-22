#ifndef _REFHASH_TEST_H
#define _REFHASH_TEST_H

#include "..\minunit.h"
#include "refhash.h"
#include "refarray.h"
#include "..\memory\manager.h"

MU_TEST(_refhash_init)
{
  memref m1 = hash_init(0);
  //should default to 1
  refhash* r1 = deref(&m1);
  // first prime is 3
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 0);

  memref m2 = hash_init(900);
  refhash* r2 = deref(&m2);
  mu_check(ra_count(r2->buckets) == 919);
  mu_check(hash_count(m2) == 0);

}

MU_TEST(_refhash_int_key_value)
{
  memref m1 = hash_init(0);
  refhash* r1 = deref(&m1);
  memref m2 = int_to_memref(42);
  memref m3 = int_to_memref(58);
  hash_set(m1,m2,m3);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 1);
  mu_check(hash_contains(m1,m2) != 0);
  memref m4 = hash_get(m1,m2);
  mu_check(m4.type == Int32);
  mu_check(m4.data.i == m3.data.i);
  mu_check(m4.data.i == 58);
  key_value* k = hash_get_kvp(m1,m2);
  mu_check(k->key.data.i == m2.data.i);
  mu_check(k->val.data.i == m3.data.i);
}
        
MU_TEST(_refhash_str_key_value)
{
  memref m1 = hash_init(0);
  refhash* r1 = deref(&m1);
  memref m2 = ra_init_str("hello world");
  memref m3 = ra_init_str("squirrels are the best");
  hash_set(m1,m2,m3);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 1);
  mu_check(hash_contains(m1,m2) != 0);
  memref m4 = hash_get(m1,m2);
  mu_check(m4.type == String);
  mu_check(m4.data.i == m3.data.i);
  key_value* k = hash_get_kvp(m1,m2);
  mu_check(k->key.data.i == m2.data.i);
  mu_check(k->val.data.i == m3.data.i);
}

MU_TEST(_refhash_replace)
{
  memref m1 = hash_init(0);
  refhash* r1 = deref(&m1);
  memref m2 = int_to_memref(42);
  memref m3 = int_to_memref(58);
  memref m4 = int_to_memref(128);
  hash_set(m1,m2,m3);

  hash_set(m1,m2,m4);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 1);
  mu_check(hash_contains(m1,m2) != 0);
  memref m5 = hash_get(m1,m2);
  mu_check(m5.type == Int32);
  mu_check(m5.data.i == m4.data.i);
  mu_check(m5.data.i == 128);
}

MU_TEST(_refhash_resize)
{
  memref k = int_to_memref(1);
  memref k1 = int_to_memref(2);
  memref k2 = int_to_memref(3);
  memref k3 = int_to_memref(4);
  memref k4 = int_to_memref(5);
  memref k5 = int_to_memref(6);

  memref m1 = hash_init(0);
  refhash* r1 = deref(&m1);
  hash_set(m1,k,k);
  hash_set(m1,k1,k1);
  hash_set(m1,k2,k2);
  hash_set(m1,k3,k3);
  hash_set(m1,k4,k4);
  hash_set(m1,k5,k5);

  mu_check(hash_count(m1) == 6);
  mu_check(ra_count(r1->buckets) == 7);
  mu_check(hash_get(m1,k).data.i == k.data.i);
  mu_check(hash_get(m1,k1).data.i == k1.data.i);
  mu_check(hash_get(m1,k2).data.i == k2.data.i);
  mu_check(hash_get(m1,k3).data.i == k3.data.i);
  mu_check(hash_get(m1,k4).data.i == k4.data.i);
  mu_check(hash_get(m1,k5).data.i == k5.data.i);
}

MU_TEST(_refhash_remove)
{
  memref m1 = hash_init(0);
  refhash* r1 = deref(&m1);
  memref m2 = int_to_memref(42);
  memref m3 = int_to_memref(58);

  //should do nothing
  hash_remove(m1,m2);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 0);
  
  hash_set(m1,m2,m3);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 1);

  hash_remove(m1,m2);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 0);
  
  hash_remove(m1,m2);
  mu_check(ra_count(r1->buckets) == 3);
  mu_check(hash_count(m1) == 0);

}
MU_TEST(_refhash_resize_remove)
{
  memref k = int_to_memref(1);
  memref k1 = int_to_memref(2);
  memref k2 = int_to_memref(3);
  memref k3 = int_to_memref(4);
  memref k4 = int_to_memref(5);
  memref k5 = int_to_memref(6);

  memref m1 = hash_init(0);
  refhash* r1 = deref(&m1);
  hash_set(m1,k,k);
  hash_set(m1,k1,k1);
  hash_set(m1,k2,k2);
  hash_set(m1,k3,k3);
  hash_set(m1,k4,k4);
  hash_set(m1,k5,k5);

  hash_remove(m1,k);
  hash_remove(m1,k1);
  mu_check(hash_count(m1) == 4);
  mu_check(ra_count(r1->buckets) == 7);

  mu_check(hash_get(m1,k2).data.i == k2.data.i);
  mu_check(hash_get(m1,k3).data.i == k3.data.i);
  mu_check(hash_get(m1,k4).data.i == k4.data.i);
  mu_check(hash_get(m1,k5).data.i == k5.data.i);

  mu_check(hash_contains(m1,k) == 0);
  mu_check(hash_contains(m1,k1) == 0);
  
}


MU_TEST(refhash_refcounts)
{
  
  
}

MU_TEST_SUITE(refhash_suite)
{
  MU_RUN_TEST(_refhash_init);
  MU_RUN_TEST(_refhash_int_key_value);
  MU_RUN_TEST(_refhash_str_key_value);
  MU_RUN_TEST(_refhash_replace);
  MU_RUN_TEST(_refhash_remove);
  MU_RUN_TEST(_refhash_resize);
  MU_RUN_TEST(_refhash_resize_remove);
  
}

#endif
