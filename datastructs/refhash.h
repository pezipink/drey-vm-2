#ifndef _REFHASH_H
#define _REFHASH_H
#include "..\memory\manager.h"
#include "..\global.h"

typedef struct refhash
{
  int id;
  stringref location_key;
  stringref visibility;
  int kvp_count;
  memref buckets;
} refhash;

memref hash_init(unsigned initial_size);

memref hash_get(memref hash, memref key);

int hash_try_get(memref* result, memref hash, memref key);

bool hash_contains(memref hash, memref key);

key_value* hash_get_kvp(memref hash, memref key);

void hash_set(memref hash, memref key, memref value);

void hash_remove(memref hash, memref key);

int hash_count(memref hash);

void hash_free(memref hash);

#endif
