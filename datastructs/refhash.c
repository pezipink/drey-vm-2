#include <windows.h>
#include <assert.h>
#include "refhash.h"
#include "..\global.h"
#include "..\memory\manager.h"
#include "..\memory\fixed_pool.h"
#include "..\datastructs\refarray.h"

static unsigned int primes[72] =  {  
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


memref hash_init(unsigned initial_size)
{
  TL("hash init entry");
  if(initial_size < 1)
    {
      initial_size = 1;
    }
  int hash_off = fixed_pool_alloc(hash_memory);
  refhash* hash = (refhash*)fixed_pool_get(hash_memory,hash_off);
  unsigned actual_size = get_next_prime(initial_size);
  memref bucket_ref = ra_init(sizeof(memref),actual_size);
  ra_consume_capacity(bucket_ref);
  hash->id = max_hash_id++;
  hash->kvp_count = 0;
  hash->buckets = bucket_ref;
  memref ref = malloc_ref(Hash, hash_off);
  TL("hash init exit");
  return ref;
}

int hash_try_get(memref* result, memref hash, memref key)
{
  TL("hash_try_get enter\n");
  TL("hash type %i\n", hash.type);
  refhash* h = (refhash*)deref(&hash);
  unsigned hash_val = memref_hash(key);
  memref bucket_ref = h->buckets;
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  memref bucket = ra_nth_memref(bucket_ref, bucket_index);
  if(bucket.type == 0)
    {
      return 0;
    }
  key_value* data = (key_value*)deref(&bucket);

  while(data->next.type != 0)
    {
      if(memref_equal(key,data->key))
        {
          *result = data->val;
          return 1;
        }
      data = (key_value*)deref(&data->next);
    }
  if(memref_equal(key,data->key))
    {
      *result = data->val;
      return 1;
    }
  
  TL("hash_try_get exit\n");
  return 0;
 
}


void print_hash(memref hashref, int indent)
{  
  refhash* hash = deref(&hashref);
  int bucketCount = ra_count(hash->buckets);
  key_value* kvp = 0;
  int eleCount = 0;
  printf("%*s{\n", indent, "");
  for(int i = 0; i < bucketCount; i++)
    { 
      memref r = ra_nth_memref(hash->buckets, i);
      if(r.type == KVP)
        {
          while(true)
            {
              kvp = deref(&r);
              if(kvp->key.type == String)
                {                  
                  ra_w_i(kvp->key, indent);
                }              
              else
                {
                  printf("%*i",indent, kvp->key.data.i);
                }
              printf(" : ");
              if(kvp->val.type == String)
                {                  
                  ra_w(kvp->val);
                }
              else if(kvp->val.type == GameObject)
                {
                  printf("\n");
                  gameobject* gop = deref(&kvp->val);
                  print_hash(gop->props, indent + 4);
                }
              else
                {
                  printf("%*i",indent,kvp->val.data.i);
                }

              printf("\n");
                

              eleCount++;
              if(kvp->next.type == KVP)
                {
                  r = kvp->next;
                }
              else
                {
                  break;
                }
            }
        }
      if(eleCount == hash->kvp_count)
        {
          break;
        }
    }
    printf("%*s}\n", indent, "");
}

memref hash_get(memref hash, memref key)
{
  TL("hash_get enter\n");
  TL("hash type %i\n", hash.type);
  refhash* h = (refhash*)deref(&hash);
  TL("hashing..\n");
  unsigned hash_val = memref_hash(key);
  memref bucket_ref = h->buckets;
  TL("BUCKET COUNT IS %i\n", ra_count(bucket_ref));
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  TL("bucket index is %i", bucket_index);
  memref bucket = ra_nth_memref(bucket_ref, bucket_index);
  key_value* data = (key_value*)deref(&bucket);
  //TL("hash_get next_off is %i\n", data->next);
  while(data->next.type != 0)
    {
      if(memref_equal(key,data->key))
        {
          return data->val;
        }
      data = (key_value*)deref(&data->next);
    }

  if(!memref_equal(key,data->key))
    {
      printf("couldn't find key ");
      ra_wl(key);
      print_hash(hash,0);
    }
  assert(memref_equal(key,data->key));
  TL("hash_get exit\n");
  return data->val;
  
}

int hash_count(memref hash)
{
 refhash* h = (refhash*)deref(&hash);
 return h->kvp_count;
}

bool hash_contains(memref hash, memref key)
{
  refhash* h = (refhash*)deref(&hash);
  unsigned hash_val = memref_hash(key);
  memref bucket_ref = h->buckets;  
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  memref kvp_ref = ra_nth_memref(bucket_ref, bucket_index);

  if(kvp_ref.type == 0)
    {
      return false;
    }
  key_value* data = (key_value*)deref(&kvp_ref);
  
  while(data->next.type != 0)
    {
      if(memref_equal(key,data->key))
        {
          return true;
        }
      data = (key_value*)deref(&data->next);
    }
  
  if(memref_equal(key,data->key))
    {
      return true;
    }

  return false;
}

//DO NOT STORE THIS POINTER!
key_value* hash_get_kvp(memref hash, memref key)
{
  refhash* h = (refhash*)deref(&hash);
  unsigned hash_val = memref_hash(key);
  memref bucket_ref = h->buckets;
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  memref bucket = ra_nth_memref(bucket_ref, bucket_index);
  key_value* data = (key_value*)deref(&bucket);
  while(data->next.type != 0)
    {
      if(memref_equal(key,data->key))
        {
          return data;
        }
      data = (key_value*)deref(&data->next);
    }
  return data;
}

//returns a new ra with the values
memref hash_get_values(memref hashref)
{
  refhash* hash = deref(&hashref);
  int bucketCount = ra_count(hash->buckets);
  memref ra_ret = ra_init(sizeof(memref),hash->kvp_count);
  ra_consume_capacity(ra_ret);
  key_value* kvp = 0;
  int eleCount = 0;
  for(int i = 0; i < bucketCount; i++)
    { 
      memref r = ra_nth_memref(hash->buckets, i);
      if(r.type == KVP)
        {
          while(true)
            {
              kvp = deref(&r);                  
              ra_set_memref(ra_ret, eleCount, &kvp->val);
              eleCount++;
              if(kvp->next.type == KVP)
                {
                  r = kvp->next;
                }
              else
                {
                  break;
                }
            }
        }
      if(eleCount == hash->kvp_count)
        {
          break;
        }
    }
  return ra_ret;
}

//returns a new ra with the keys
memref hash_get_keys(memref hashref)
{
  refhash* hash = deref(&hashref);
  int bucketCount = ra_count(hash->buckets);
  memref ra_ret = ra_init(sizeof(memref),hash->kvp_count);
  ra_consume_capacity(ra_ret);
  key_value* kvp = 0;
  int eleCount = 0;
  for(int i = 0; i < bucketCount; i++)
    { 
      memref r = ra_nth_memref(hash->buckets, i);
      if(r.type == KVP)
        {
          while(true)
            {
              kvp = deref(&r);
              //              ra_wl(kvp->key);
              ra_set_memref(ra_ret, eleCount, &kvp->key);
              eleCount++;
              if(kvp->next.type == KVP)
                {
                  r = kvp->next;
                }
              else
                {
                  break;
                }
            }
        }
      if(eleCount == hash->kvp_count)
        {
          break;
        }
    }
  return ra_ret;
}

//returns a ra with the key_value refs
//DO NOT STORE THESE KEY_VALUES!!
memref hash_get_key_values(memref hashref)
{
  refhash* hash = deref(&hashref);
  int bucketCount = ra_count(hash->buckets);
  memref ra_ret = ra_init(sizeof(memref),hash->kvp_count);
  ra_consume_capacity(ra_ret);
  key_value* kvp = 0;
  int eleCount = 0;
  for(int i = 0; i < bucketCount; i++)
    { 
      memref r = ra_nth_memref(hash->buckets, i);
      if(r.type == KVP)
        {
          while(true)
            {
              kvp = deref(&r);
              ra_set_memref(ra_ret, eleCount, &r);
              eleCount++;
              if(kvp->next.type == KVP)
                {
                  r = kvp->next;
                }
              else
                {
                  break;
                }
            }
        }
      if(eleCount == hash->kvp_count)
        {
          break;
        }
    }
  return ra_ret;
}


void hash_remove(memref hash, memref key)
{
  TL("hash_remove enter\n");
  refhash* h = (refhash*)deref(&hash);
  unsigned hash_val = memref_hash(key);
  memref bucket_ref = h->buckets;
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  memref curr_ref = ra_nth_memref(bucket_ref, bucket_index);
  if(curr_ref.type == 0)
    {
      DL("key not found in hash_remove\n");
      return;
    }
  key_value* data = (key_value*)deref(&curr_ref);
  key_value* prev = NULL;

  char found = 0;
  while(data->next.type != 0)
    {
      if(memref_equal(key,data->key))
        {
          found = 1;
          break;
        }
      prev = data;
      curr_ref = data->next;
      data = (key_value*)deref(&curr_ref);
    }

  if(found == 0)
    {
      if(memref_equal(key,data->key) == 0)
        {
          DL("key not found in hash_remove\n");
          return;
        }
    }
  
  h->kvp_count--;
  if(prev == NULL && data->next.type != 0)
    {
      TL("cleared bucket list\n");
      //was the only item in the list, clear it
      memref r;
      r.type = 0;
      ra_set(bucket_ref,bucket_index,&r);
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
  TL("hash key count currently %i\n",h->kvp_count);
  unsigned new_size = get_next_prime(h->kvp_count);
  TL("reszing to %i elements\n",new_size);

  memref new_buckets = ra_init(sizeof(memref),new_size);
  ra_consume_capacity(new_buckets);

  TL("resizing hash with kvp count %i from %i to %i buckets\n", h->kvp_count, ra_count(h->buckets), new_size);
  int count = ra_count(h->buckets);
  for(int i = 0; i < count; i++)
    {
      TL("processing bucket %i\n", i);
      memref item = ra_nth_memref(h->buckets,i);
      while(item.type > 0)
        {
          TL("derefing item with targ offset of %i\n", get_ref(item.data.i)->targ_off);
          key_value* kvp = deref(&item);
          TL("kvp->key = %i\n",kvp->key);
          TL("kvp->value = %i\n",kvp->val);
          TL("kvp->next = %i\n",kvp->next);
          //rehash and insert into new list
          unsigned hash_val = memref_hash(kvp->key);
          TL("hashed value is %i\n",hash_val);
          unsigned bucket_index = hash_val % new_size;
          TL("new bucket for key is %i\n", bucket_index);
          memref current = ra_nth_memref(new_buckets,bucket_index);
          memref next = kvp->next;

          kvp->next = current;
          ra_set(new_buckets,bucket_index,&item);
          
          TL("kvp->next = %i\n",kvp->next);
          item = next;
          //          TL("item is %i", next == NULL);

          TL("LOOP\n");
        }
    }

  
  /* TL("freeing old buckets\n"); */
  /* dyn_pool_free(dyn_memory,get_ref(h->buckets.data.i)->targ_off); */
  TL("assigning new bucket list\n");
  h->buckets = new_buckets;
  TL("hash_resize exit\n");
}


void hash_set(memref hash, memref key, memref value)
{
  TL("hash_set enter\n");
  TL("hash type %i\n", hash.type);

  refhash* h = (refhash*)deref(&hash);
  TL("hash_srt current kvp count %i\n", h->kvp_count);
  unsigned hash_val = memref_hash(key);
  TL("bucket ref offset is %p\n",h->buckets);
  memref bucket_ref = h->buckets;
  //  TL("bucket ref is of type %i with target %i\n", bucket_ref.type, bucket_ref.data.r->targ_off);
  unsigned bucket_index = hash_val % (unsigned)ra_count(bucket_ref);
  TL("bucket index is %i\n",bucket_index);
  memref kvp_curr_ref = ra_nth_memref(bucket_ref, bucket_index);
  memref kvp_new_ref;
  kvp_new_ref.type = 0;
  char replaced = 0;
  TL("kvp_curr_ref is %i\n",kvp_curr_ref);
  if(kvp_curr_ref.type == 0)
    {
      TL("setting new hash key!\n");
      // new list head
      memref next;
      next.type = 0;
      kvp_new_ref = malloc_kvp(key,value, next);      
    }
  else
    {
      // see if this key exists, and replace it if so
      key_value* data = (key_value*)deref(&kvp_curr_ref);
      if(memref_equal(key,data->key))
        {
          replaced = 1;
        }
      else if(data->next.type != 0)
        {      
          while(data->next.type != 0)
          {
            if(memref_equal(key,data->key))
              {
                replaced = 1;
                break;
              }
            data = (key_value*)deref(&data->next);
          }
        }

      if(replaced == 1)
        {
          TL("replacing key in hash\n");
          data->val = value;
        }
      else
        {
          TL("hash collission, adding to existing list at bucket %i\n", bucket_index);
          kvp_new_ref = malloc_kvp(key,value, kvp_curr_ref);   
        }
            
    }

  if(replaced == 0)
    {
      TL("increasing refs\n");
      TL("setting..\n");
      kvp_new_ref.type = KVP;
      ra_set(bucket_ref,bucket_index,&kvp_new_ref);
      h->kvp_count++;
      //resize and redistribute if not enough buckets.
      if(h->kvp_count > ra_count(bucket_ref))
        {
          TL("reszing..\n");
          hash_resize(h);
        }

    }
  TL("hash_set exit\n");
}



void hash_free(ref* hash_ref)
{
  TL("hash_free enter\n");
  refhash* h = fixed_pool_get(hash_memory, hash_ref->targ_off);

  fixed_pool_free(hash_memory,hash_ref->targ_off);
  TL("hash_free exit\n");
}
