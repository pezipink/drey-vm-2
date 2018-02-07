#ifndef _REFHASH_H
#define _REFHASH_H
#include "..\memory\manager.h"
#include "..\global.h"

typedef struct key_value
{
  memref* key;
  memref* value;
  struct key_value* next;
} key_value;

typedef struct refhash
{
  int id;
  stringref location_key;
  stringref visibility;
  int kvp_count;
  int bucket_count;
  int bucket_off;
} refhash;

memref* hash_init(int initial_size);

memref* hash_get(memref* hash, memref* key);

bool hash_contains(memref* hash, memref* key);

key_value* hash_get_kvp(memref* hash, memref* key);

void hash_set(memref* hash, memref* key, memref* value);




#endif
