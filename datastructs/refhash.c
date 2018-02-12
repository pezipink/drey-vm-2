#include <windows.h>
#include "refhash.h"
#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"
#include "..\datastructs\refarray.h"

static int primes[72] =  {  
        3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
        1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
        17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
        187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
        1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369};  

memref* malloc_kvp(memref* key, memref* val, memref* next)
{
  TL("malloc_kvp entry\n");
  //allocate int
  int kvp_off = fixed_pool_alloc(kvp_memory);
  key_value* kvp_val = (key_value*)fixed_pool_get(kvp_memory,kvp_off);
  kvp_val->key = key;
  kvp_val->val = val;
  kvp_val->next = next;
  //alloc ref to it
  memref* ref = malloc_ref(KVP,kvp_off);
  TL("malloc_kvp exit\n");
  return ref;
}


memref* hash_init(int initial_size)
{
  TL("hash init entry");
  int hash_off = fixed_pool_alloc(hash_memory);
  refhash* hash = (refhash*)fixed_pool_get(hash_memory,hash_off);
  memref* bucket_ref = ra_init(sizeof(memref*),initial_size);
  bucket_ref->refcount++;  
  hash->id = max_hash_id++;
  hash->kvp_count = 0;
  hash->buckets = bucket_ref;
  memref* ref = malloc_ref(Hash, hash_off);
  TL("hash init exit");
  return ref;
}


memref* hash_get(memref* hash, memref* key)
{
  TL("hash_get enter\n");
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;
  TL("BUCKET COUNT IS %i\n", ra_count(bucket_ref));
  int bucket_index = hash_val % ra_count(bucket_ref);
  key_value* data = (key_value*)deref(ra_nth_memref(bucket_ref, bucket_index));
  //TL("hash_get next_off is %i\n", data->next);
  while(data->next != NULL)
    {
      if(memref_equal(key,data->key))
        {
          return data->val;
        }
      data = (key_value*)deref(data->next);
    }
  TL("hash_get exit\n");
  return data->val;
  
}

int hash_count(memref* hash)
{
 refhash* h = (refhash*)deref(hash);
 return h->kvp_count;
}

bool hash_contains(memref* hash, memref* key)
{
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;  
  int bucket_index = hash_val % ra_count(bucket_ref);
  memref* kvp_ref = ra_nth_memref(bucket_ref, bucket_index);

  if(kvp_ref == NULL)
    {
      return false;
    }
  key_value* data = (key_value*)deref(kvp_ref);
  
  while(data->next != NULL)
    {
      if(memref_equal(key,data->key))
        {
          return true;
        }
      data = (key_value*)deref(data->next);
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
  memref* bucket_ref = h->buckets;
  int bucket_index = hash_val % ra_count(bucket_ref);
  key_value* data = (key_value*)deref(ra_nth_memref(bucket_ref, bucket_index));
  while(data->next != NULL)
    {
      if(memref_equal(key,data->key))
        {
          return data;
        }
      data = (key_value*)deref(data->next);
    }
  return data;
}

void hash_remove(memref* hash, memref* key)
{
  TL("hash_remove enter\n");
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;
  int bucket_index = hash_val % ra_count(bucket_ref);
  memref* curr_ref = ra_nth_memref(bucket_ref, bucket_index);
  key_value* data = (key_value*)deref(curr_ref);
  key_value* prev = NULL;

  while(data->next != NULL)
    {
      prev = data;
      if(memref_equal(key,data->key))
        {
          break;
        }
      curr_ref = data->next;
      data = (key_value*)deref(curr_ref);
    }
  
  h->kvp_count--;
  curr_ref->refcount--;
  data->key->refcount--;
  data->val->refcount--;
  if(prev == NULL && data->next != NULL)
    {
      TL("cleared bucket list\n");
      //was the only item in the list, clear it
      ra_set(bucket_ref,bucket_index,(memref*)0);
    }
  else if(prev == NULL)
    {
      TL("assigned new bucket list head\n");
      //assign the second item as the new head
      ra_set(bucket_ref,bucket_index,&data->next);
    }
  else
    {
      TL("removed bucket from list\n");
      prev->next = data->next;
    }
  TL("hash_remove enter\n");
}


void hash_set(memref* hash, memref* key, memref* value)
{
  TL("hash_set enter\n");
  refhash* h = (refhash*)deref(hash);
  int hash_val = memref_hash(key);
  TL("bucket ref offset is %p\n",h->buckets);
  memref* bucket_ref = h->buckets;
  TL("bucket ref is of type %i with target %i\n", bucket_ref->type, bucket_ref->targ_off);
  int bucket_index = hash_val % ra_count(bucket_ref);
  TL("bucket index is %i\n",bucket_index);
  memref* kvp_ref = ra_nth_memref(bucket_ref, bucket_index);
  TL("kvp_ref is %i\n",kvp_ref);
  if(kvp_ref == NULL)
    {
      memref* kvp_ref = malloc_kvp(key,value, (memref*)0);
      key->refcount++;
      value->refcount++;
      kvp_ref->refcount++;
      ra_set(bucket_ref,bucket_index,&kvp_ref);
    }
  else
    {
      TL("!! NOT IMPLEMENTED\n");
    }

  h->kvp_count++;

  //TODO: resize and redistribute if not enoug buckets.
  
  TL("hash_set exit\n");
}

