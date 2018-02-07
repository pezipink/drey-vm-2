#include "refhash.h"
#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"

static int primes[72] =  {  
        3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
        1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
        17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
        187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
        1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369};  

memref* hash_init(int initial_size)
{
  TL("hash init entry");
  int hash_off = fixed_pool_alloc(hash_memory);
  refhash* hash = (refhash*)fixed_pool_get(hash_memory,hash_off);
  int bucket_off = dyn_pool_alloc_set(&dyn_memory, sizeof(key_value) * 3, 0);
  hash->id = max_hash_id++;
  hash->kvp_count = 0;
  hash->bucket_count = 3;
  hash->bucket_off = bucket_off;
  memref* ref = malloc_ref(Hash, hash_off);
  TL("hash init exit");
  return ref;
}


memref* hash_get(memref* hash, memref* key)
{
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  int bucket = hash_val % h->bucket_count;
  key_value* data = (key_value*)dyn_pool_get(dyn_memory,h->bucket_off);
  data += bucket;
  while(data->next != 0)
    {
      if(memref_equal(key,data->key))
        {
          return data->value;
        }
      data = data->next;          
    }
  return data->value;
}

bool hash_contains(memref* hash, memref* key)
{
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  int bucket = hash_val % h->bucket_count;
  key_value* data = (key_value*)dyn_pool_get(dyn_memory,h->bucket_off);
  data += bucket;

  if(*(int*)data == 0)
    {
      return false;
    }
  
  while(data->next != 0)
    {
      if(memref_equal(key,data->key))
        {
          return true;
        }
      data = data->next;          
    }
  
  if(memref_equal(key,data->key))
    {
      return true;      
    }

  return false;
}


key_value* hash_get_kvp(memref* hash, memref* key)
{
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  int bucket = hash_val % h->bucket_count;
  key_value* data = (key_value*)dyn_pool_get(dyn_memory,h->bucket_off);
  data += bucket;
  while(data->next != 0)
    {
      if(memref_equal(key,data->key))
        {
          return data;
        }
      data = data->next;          
    }
  return data;
}

void hash_set(memref* hash, memref* key, memref* value)
{
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  int bucket = hash_val % h->bucket_count;
  key_value* data = (key_value*)dyn_pool_get(dyn_memory,h->bucket_off);
  data += bucket;
  if(data->key == 0)
    {
      key_value kvp;
      kvp.key = key;
      kvp.key->refcount++;
      kvp.value = value;
      kvp.value->refcount--;
      *data = kvp;
    }
  
}

/* memref* malloc_go() */
/* { */
/*   printf("malloc_go entry\n"); */
/*   int go_off = fixed_pool_alloc(go_memory); */
/*   go* go_val = (go*)fixed_pool_get(go_memory,go_off); */
/*   go_val->id = max_go_id++; */
/*   go_val->kvp_count = 0; */
/*   go_val->bucket_count = 3; */
/*   go_val->buckets = 0;// todo: alloc dynamic space for kvp array */
/*   memref* ref = malloc_ref(GameObject,go_off); */
/*   printf("malloc_go exit\n"); */
/*   return ref; */
/* } */

