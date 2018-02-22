#include "gc.h"
#include "manager.h"
#include "..\global.h"

void gc_print_stats()
{
  int fcounts[40];
  int ucounts[40];
  for(int i = 0; i < 40; i++)
    {
      fcounts[i]=0;
      ucounts[i]=0;
    }
  int max = ref_memory->element_count;
  int toFree = 0;
  int inUse = 0;
  int unused = 0;
  ref* item = (ref*)&ref_memory->data;
  int offset = 0;
  for(int i = 0; i < max; i++)
    {
      if(item->type > 0)
        {
          if(item->refcount == 0)
            {
              fcounts[item->type]++;
            }
          else
            {
              ucounts[item->type]++;
            }
        }
      else
        {
          unused++;
        }
      item++;
      offset+=sizeof(ref);
    }
  DL("GC STATS:\n");
  DL("Hash\t to free %i\tin use %i\n",fcounts[Hash],ucounts[Hash]);
  DL("Array\t to free %i\tin use %i\n",fcounts[Array],ucounts[Array]);
  DL("String\t to free %i\tin use %i\n",fcounts[String],ucounts[String]);
  DL("KVP\t to free %i\tin use %i\n",fcounts[KVP],ucounts[KVP]);
  DL("Stack\t to free %i\tin use %i\n",fcounts[Stack],ucounts[Stack]);
    
 }

int gc_clean_full()
{
  int cycles = 1;
  DL("iteration 0\n");
  while(gc_clean_sweep())
    {
      DL("iteration %i\n",cycles);
      cycles ++;
    }
  return cycles;
}

int gc_clean_sweep()
{
  DL("gc_clean_full enter\n");
  // single sweep and free on all references
  int offset = 0;
  int max = ref_memory->element_count;
  int toFree = 0;
  int inUse = 0;
  int unused = 0;
  ref* item = (ref*)&ref_memory->data;
  for(int i = 0; i < max; i++)
    {
      if(item->type != 0)
        {
          if(item->refcount == 0)
            {
              toFree++;
              free_ref(item,offset);
            }
          else
            {
              inUse++;
            }
        }
      else
        {
          unused++;
        }
      item++;
      offset+=sizeof(ref);
    }
  // DL("found %i references to free, %i still in use and %i unused\n", toFree, inUse, unused);
  return toFree;
}

int gc_clean_step(int last_offset)
{
  DL("gc_clean_step enter\n");
  int steps = ref_memory->element_count / 10;
  if(steps < 10)
    {
      steps = 10;
    }
  DL("stepping %i times from offset %i\n",steps,last_offset);
  int toFree = 0;
  ref* item = (ref*)((int)&ref_memory->data + last_offset);
  int maxOffset = ref_memory->element_count * sizeof(ref);
  int newOffset = last_offset;
  for(int i = 0; i < steps; i++)
    {
      if(item->type != 0)
        {
          if(item->refcount == 0)
            {
              toFree++;
              free_ref(item,newOffset);
            }
        }

      if(newOffset == maxOffset)
        {
          newOffset = 0;
          item = (ref*)&ref_memory->data;
        }
      else
        {
          item++;
          newOffset+=sizeof(ref);
        }

    }
  DL("finished gc_step, freed %i items ending at offset %i\n", toFree, newOffset);
  DL("gc_clean_step exit\n");
  return newOffset;
}


void gc_clean_cycles()
{
  // cycle detection
}
