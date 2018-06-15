#include "json.h"
#include "refarray.h"
#include "refhash.h"
#include "..\global.h"
#include "..\memory\manager.h"


// a unoptimised, terribly minimal json parser that doesn't even pretend to know about the standard


static inline int skip_ws(char**data, int *offset, int len)
{  
    if (*offset > len)
    {
        printf("invalid json code 1");
        return 0;
    }
    while (isspace(**data))
    {
        (*data)++;
        (*offset)++;
        if (*offset > len)
        {
            printf("invalid json code 1");
            return 0;
        }
    }
    return 1;
}


#define FWD (*data)++; (*offset)++; 
#define BOUNDS_CHECK if(*offset>len) {printf("invalid json code 58\n"); return nullref(); };
#define SKIP_WS if(!skip_ws(data,offset,len)){return nullref();}

static memref json_to_object_internal(char** data, int* offset, int len)
{
    // incoming message strings from zmq are not null terminated, hence the BOUNDS_CHECK marco along with the
    // counters assert we haven't ended up past the end of the string with bad json
    SKIP_WS;
    if (**data == '[')
    {
        FWD;
        BOUNDS_CHECK;
        memref ra = ra_init(sizeof(memref), 3);
        while (**data != ']')
        {
            memref next = json_to_object_internal(data, offset, len);
            ra_append_memref(ra, next);
            SKIP_WS;
            if (**data == ',')
            {
                FWD;
            }
            else if (**data != ']')
            {
                printf("invalid json code 3 %c\n", **data);
                return nullref();
            }
        }

        FWD;
        return ra;

    }
    else if (**data == '{')
    {
        FWD;
        memref rh = hash_init(3);
        BOUNDS_CHECK;
        while (**data != '}')
        {
            memref key = json_to_object_internal(data, offset, len);
            if (key.type != String)
            {
                printf("invalid json code 4 key was not a string\n");
                return nullref();
            }
            SKIP_WS;
            if (**data != ':')
            {
                printf("invalid json code 5 expected : between key and value\n");
                return nullref();
            }
            FWD;
            memref value = json_to_object_internal(data, offset, len);
            hash_set(rh, key, value);
            SKIP_WS;
            if (**data == ',')
            {
                FWD;
            }
            else if (**data != '}')
            {
                printf("invalid json code 6\n");
                return nullref();
            }
        }

        // skip trailing }
        FWD;
        return rh;
    }
    else if (**data == '"')
    {
        //TDOO: support escaped quotes
        char* start = ++(*data);
        (*offset)++;
        int str_len = 0;
        while (**data != '"')
        {
            FWD; BOUNDS_CHECK;
            str_len++;
            if (*offset > len)
            {
                DIE("invalid json code 2");
            }
        }
        //skip trailing "
        FWD;
        memref ra = ra_init_str_raw(start, str_len);
        return ra;
    }
    else if (isdigit(**data))
    {
        //TODO: support hex
        //TODO: check number is not more than int characters depending on base
        char* start = *data;
        int nlen = 0;
        while (isdigit(**data))
        {
            nlen++;
            FWD;
            BOUNDS_CHECK;
        }
        if(nlen > 11)
          {
            printf("invalid number in json\n");
            return nullref();
          }
        char numstr[11];
        strncpy(numstr, start, nlen);
        int num = atoi(numstr);
        memref ret = int_to_memref(num);
        return ret;
    }
    else if (isalpha(**data))
    {
        if (**data == 't')
        {
            FWD; BOUNDS_CHECK;
            if (**data == 'r')
            {
                FWD; BOUNDS_CHECK;
                if (**data == 'u')
                {
                    FWD; BOUNDS_CHECK;
                    if (**data == 'e')
                    {
                        FWD;
                        memref ret = int_to_memref(1);
                        return ret;
                    }
                }
            }
        }
        else if (**data == 'f')
        {
            FWD; BOUNDS_CHECK;
            if (**data == 'a')
            {
                FWD; BOUNDS_CHECK;
                if (**data == 'l')
                {
                    FWD; BOUNDS_CHECK;
                    if (**data == 's')
                    {
                        FWD; BOUNDS_CHECK;
                        if (**data == 'e')
                        {
                            FWD;
                            memref ret = int_to_memref(0);
                            return ret;
                        }
                    }
                }
            }
        }

        printf("invalid json code 7\n");
        return nullref();

    }

    printf("invalid json code 42\n");
    return nullref();
}

memref json_to_object(char** data, int len)
{
    int offset = 0;
    return json_to_object_internal(data, &offset, len);
}

static stringref object_to_json_internal(memref obj, stringref json)
{
  switch(obj.type)
    {
    case Int32:
      char buf[11];
      itoa(obj.data.i, buf, 10);
      ra_append_str(json, buf, strlen(buf));
      break;
      
    case Hash:
      ra_append_char(json, '{');
      refhash* hash = deref(&obj);
      int bucketCount = ra_count(hash->buckets);
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
                  if(kvp->key.type == String)
                    {                  
                      object_to_json_internal(kvp->key, json);
                      ra_append_char(json, ':');
                      object_to_json_internal(kvp->val, json);
                    }              
                  else
                    {
                      printf("could not serialize non-string key in hash\n");
                    }

                  eleCount++;
                  if(eleCount != hash->kvp_count)
                    {
                      ra_append_char(json,',');
                    }
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
      ra_append_char(json,'}');
      
      break;
      
    case GameObject:
      //TODO:
      break;

    case Array:
      ra_append_char(json,'[');
      int len = ra_count(obj);
      for(int i = 0; i < len; i++)
        {
          if(i != 0)
            {
              ra_append_char(json,',');
            }
          memref obj2 = ra_nth_memref(obj, i);
          object_to_json_internal(obj2, json);
        }
      ra_append_char(json,']');
      break;

    case String:
      ra_append_char(json,'"');
      ra_append_ra_str(json, obj);
      ra_append_char(json,'"');
      break;
      
    default:
      DIE("Unsupported type in object_to_json\n");
      
    }
}

stringref object_to_json(memref obj)
{
  stringref json = ra_init_raw(sizeof(char), 1024, String);
  object_to_json_internal(obj, json);
  return json;  
}
