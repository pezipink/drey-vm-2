#ifndef _REFARRAY_H
#define _REFARRAY_H

#include "..\memory\manager.h"
#include "..\global.h"

typedef struct refarray
{
  int element_size;
  int element_count;
  int element_capacity;
  void* data;
} refarray;


memref ra_init_raw(unsigned element_size, unsigned element_capacity, unsigned int type);
memref ra_init(unsigned element_size, unsigned element_capacity);
memref ra_init_str(char* str);
memref ra_init_str_raw(char* str, int len);
unsigned ra_count(memref ra_ref);
void* ra_nth(memref ra_ref, unsigned nth);
void* ra_data(memref ra_ref);
void ra_append(memref ra_ref, void* new_element);
void ra_append_memref(memref ra_ref, memref new_element);
memref ra_clone(memref);
void ra_set(memref ra_ref, unsigned nth, void* new_element);
int ra_nth_int(memref ra_ref, unsigned nth);
void ra_remove(memref ra_ref, unsigned nth, unsigned amount);
memref ra_split_top(memref ra_ref, unsigned nth);
memref ra_split_bottom(memref ra_ref, unsigned nth);
void ra_append_int(memref ra_ref, int val);
void ra_append_char(memref ra_ref, char val);
void ra_free(ref* ra_ref);

memref ra_nth_memref(memref ra_ref, unsigned nth);
void ra_shuffle(memref );
void ra_wl(memref ra_ref);
void ra_w(memref ra_ref);
void ra_w_i(memref ra_ref, int indent);

#endif
