#ifndef _REFHASH_H
#define _REFHASH_H
#include "..\memory\manager.h"
#include "..\global.h"

typedef struct key_value
{
  memref* key;
  memref* val;
  memref* next;
} key_value;

typedef struct refhash
{
  int id;
  stringref location_key;
  stringref visibility;
  int kvp_count;
  memref* buckets;
} refhash;

memref* hash_init(unsigned initial_size);

memref* hash_get(memref* hash, memref* key);

bool hash_contains(memref* hash, memref* key);

key_value* hash_get_kvp(memref* hash, memref* key);

void hash_set(memref* hash, memref* key, memref* value);

void hash_remove(memref* hash, memref* key);

int hash_count(memref* hash);


#endif
