#ifndef _REFARRAY_H
#define _REFARRAY_H

#include "..\memory\manager.h"
#include "..\global.h"

typedef struct refarray
{
  int element_size;
  int element_count;
  void* data;
} refarray;


memref* ra_init(int element_szie, int element_count);
int ra_count(memref* ra_ref);
void* ra_nth(memref* ra_ref, int nth);
void ra_append(memref** ra_ref, void* new_element);
void ra_set(memref* ra_ref, int nth, void* new_element);
int ra_nth_int(memref* ra_ref, int nth);

memref* ra_nth_memref(memref* ra_ref, int nth);

void ra_append_memref(memref** ra_ref, memref** new_element);
void ra_set_memref(memref* ra_ref, int nth, memref** new_element);

#endif
