#include <windows.h>
#include "refhash.h"
#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"
#include "..\datastructs\refarray.h"

static unsigned primes[72] =  {  
        3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
        1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
        17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
        187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
        1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369};  

unsigned get_next_prime(unsigned number)
{
  for(int i = 0; i < 72; i++)
    {
      if(primes[i] > number)
        {
          return primes[i];
        }
    }
  DL("ciritical expcetion, no more hard coded prime numbers\n");
  //todo: generate primes here.
  //we should never reach 7million items in a hash though!
  return primes[71];
}

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


memref* hash_init(unsigned initial_size)
{
  TL("hash init entry");
  int hash_off = fixed_pool_alloc(hash_memory);
  refhash* hash = (refhash*)fixed_pool_get(hash_memory,hash_off);
  unsigned actual_size = get_next_prime(initial_size);
  memref* bucket_ref = ra_init(sizeof(memref*),actual_size);
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
  TL("hash type %i\n", hash->type);
  refhash* h = (refhash*)deref(hash);
  TL("hashing..\n");
  unsigned hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;
  TL("BUCKET COUNT IS %i\n", ra_count(bucket_ref));
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  TL("bucket index is %i", bucket_index);
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
  unsigned hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;  
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
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
  unsigned hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
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
  unsigned hash_val = memref_hash(key);
  memref* bucket_ref = h->buckets;
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
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


void hash_resize(refhash* h)
{
  TL("hash_resize enter\n");
  unsigned new_size = get_next_prime(h->kvp_count);
  memref* new_buckets = ra_init(sizeof(memref*),new_size);
  #if DEBUG
  if(h->buckets->refcount != 1)
    {
      DL("!! Critical error, hash bucket reference with != 1 ref count : %i\n", h->buckets->refcount);
    }
#endif
  h->buckets->refcount = 0;
  TL("resizing hash with kvp count %i from %i to %i buckets\n", h->kvp_count, ra_count(h->buckets), new_size);
  int count = ra_count(h->buckets);
  for(int i = 0; i < count; i++)
    {
      TL("processing bucket %i\n", i);
      memref* item = ra_nth_memref(h->buckets,i);
      while(item != NULL)
        {
          TL("derefing item with targ offset of %i\n", item->targ_off);
          key_value* kvp = deref(item);
          TL("kvp->key = %i\n",kvp->key);
          TL("kvp->value = %i\n",kvp->val);
          TL("kvp->next = %i\n",kvp->next);
          //rehash and insert into new list
          unsigned hash_val = memref_hash(kvp->key);
          TL("hashed value is %i\n",hash_val);
          unsigned bucket_index = hash_val % new_size;
          TL("new bucket for key is %i\n", bucket_index);
          memref* current = ra_nth_memref(new_buckets,bucket_index);
          memref* next = kvp->next;

          kvp->next = current;
          ra_set(new_buckets,bucket_index,&item);
          
          TL("kvp->next = %i\n",kvp->next);
          item = next;
          //          TL("item is %i", next == NULL);

          TL("LOOP\n");
        }
    }
  TL("assigning new bucket lis\n");
  h->buckets = new_buckets;
  h->buckets->refcount++;
  TL("hash_resize exit\n");
}

void hash_set(memref* hash, memref* key, memref* value)
{
  TL("hash_set enter\n");
  TL("hash type %i\n", hash->type);
  refhash* h = (refhash*)deref(hash);
  unsigned hash_val = memref_hash(key);
  TL("bucket ref offset is %p\n",h->buckets);
  memref* bucket_ref = h->buckets;
  TL("bucket ref is of type %i with target %i\n", bucket_ref->type, bucket_ref->targ_off);
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  TL("bucket index is %i\n",bucket_index);
  memref* kvp_curr_ref = ra_nth_memref(bucket_ref, bucket_index);
  memref* kvp_new_ref = 0;
  TL("kvp_curr_ref is %i\n",kvp_curr_ref);
  if(kvp_curr_ref == NULL)
    {
      kvp_new_ref = malloc_kvp(key,value, (memref*)0);
    }
  else
    {
      TL("hash collission, adding to existing list at bucket %i\n", bucket_index);
      kvp_new_ref = malloc_kvp(key,value, kvp_curr_ref);

    }
  key->refcount++;
  value->refcount++;
  kvp_new_ref->refcount++;
  ra_set(bucket_ref,bucket_index,&kvp_new_ref);

  h->kvp_count++;

  //resize and redistribute if not enough buckets.
  if(h->kvp_count > ra_count(bucket_ref))
    {
      hash_resize(h);
    }
  TL("hash_set exit\n");
}
