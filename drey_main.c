#include <stdio.h>
#include <windows.h>
#include "memory\fixed_pool.h"

MemoryPool_Fixed* int_memory = 0; // ints and pointers
int ref_memory = 0;
int go_memory = 0;
int dyn_memory = 0;
int max_go_id = 1;
//void drawMemory(HANDLE hNewScreenBuffer, MemoryPool* memory)
//{
  /* SMALL_RECT srctReadRect;  */
  /* SMALL_RECT srctWriteRect; */
  /* CHAR_INFO chiBuffer[24*96]; // [8][64];  */
  /* COORD coordBufSize;  */
  /* COORD coordBufCoord;  */
  /* BOOL fSuccess;  */

  /* coordBufSize.Y = 24;  */
  /* coordBufSize.X = 96; */
    
  /* coordBufCoord.X = 0;  */
  /* coordBufCoord.Y = 0;  */
    
  /* // Set the destination rectangle.  */
    
  /* srctWriteRect.Top = 2;    // top lt: row 10, col 0  */
  /* srctWriteRect.Left = 1;  */
  /* srctWriteRect.Bottom = 24; // bot. rt: row 11, col 79  */
  /* srctWriteRect.Right = 96;  */
  /* for(int i = 0; i < (24*96); i++) */
  /*   { */
  /*     chiBuffer[i].Char.AsciiChar = ' '; */
  /*     chiBuffer[i].Attributes = 0; */
  /*   } */
    
  /* if(memory->freeBlock == NULL) */
  /*   { */
  /*     printf("here!"); */
  /*     for(int i = 0; i < (24*96); i++) */
  /*       { */
  /*         chiBuffer[i].Char.AsciiChar = ' ';     */
  /*         chiBuffer[i].Attributes = BACKGROUND_RED; */
  /*       } */
  /*   } */
  /* else */
  /*   { */
  /*     //      printf("not null\n"); */
  /*     FreeBlock* current = memory->freeBlock; */
  /*     int start = (int)&memory->data; */
  /*     //      printf("start at %i first free block at %i\n",start,(int)current); */
  /*     int index = 0; */
  /*     if((int)current != start) */
  /*       { */
           
  /*         //print initial alloc special case */
  /*         int gap = (int)current - start; */
  /*         //  printf("start alloc %i\n",gap); */
  /*         while(index < gap) */
  /*           { */
  /*             chiBuffer[index].Char.AsciiChar = ' ';     */
  /*             chiBuffer[index].Attributes = BACKGROUND_RED; */
  /*             index++; */
  /*           }                         */
  /*       } */
  /*     //      printf("!\n"); */
  /*     //print the current free block */
  /*     while(TRUE) */
  /*       { */
  /*         printf("loop\n"); */
  /*         int targ = index + current->size; */
  /*         printf("targ %i\n",targ); */
  /*         while(index < targ) */
  /*           { */
  /*             chiBuffer[index].Attributes = BACKGROUND_GREEN; */
  /*             index++; */
  /*           } */
  /*         printf("loop2\n"); */
  /*         if(current->next == NULL) */
  /*           { */
  /*             printf("breaking\n"); */
  /*             break; */
  /*           } */
  /*         else */
  /*           { */
                            
  /*             targ = index + (int)current->next - (int)current - current->size; */
  /*             /\* printf("! %i - %i + %i", (int)current->next, (int)current, current->size); *\/ */
  /*             printf("targ %i\n",targ);  */
  /*             while(index < targ) */
  /*               { */
  /*                 chiBuffer[index].Attributes = BACKGROUND_RED; */
  /*                 index++; */
  /*               }                 */

  /*             printf("setting next\n"); */
  /*             current = current->next; */
  /*           } */
  /*       } */
  /*     //print the rest as allocated */
  /*         printf("index now %i\n",index); */
  /*     while(index < 24*96) */
  /*       { */
  /*         chiBuffer[index].Attributes = BACKGROUND_RED; */
  /*         index++;            */
  /*       } */
  /*   } */
        
  /* printf("printing HERE FIAL\n"); */
  /* // Copy from the temporary buffer to the new screen buffer.  */
    
  /* fSuccess = WriteConsoleOutput( */
  /*                               hNewScreenBuffer, // screen buffer to write to */
  /*                               chiBuffer,        // buffer to copy from */
  /*                               coordBufSize,     // col-row size of chiBuffer */
  /*                               coordBufCoord,    // top left src cell in chiBuffer */
  /*                               &srctWriteRect);  // dest. screen buffer rectangle */
  /* if (! fSuccess) */
  /*   { */
  /*     printf("WriteConsoleOutput failed - (%d)\n", GetLastError()); */
  /*   } *
  /* printf("exit"); */
//}


#define Int32 1
#define Bool 2
#define Double 3
#define GameObject 4
#define String 5
#define List 6
#define Function 7


typedef struct memref
{
  char type;
  short refcount;
  int ref_off;
  int targ_off;
  
} memref, stringref, intref;

typedef struct kvp
{
  memref key;
  memref value;
  struct kvp* next; 
} kvp;

typedef struct go
{
  int id;
  stringref location_key;
  stringref visibility;
  int kvp_count;
  int bucket_count;
  int* buckets;
} go, dict;


typedef struct refstack
{
  // stack has its own static pool of memory
  // since we only ever push to the top,
  // we know it wont "fragment" at the bottom,
  // and it will be auto resized 
  int pool;
  int head_offset;
} refstack;


refstack stack_init(int initialSize)
{
  printf("stack_init entry\n");
  refstack stack;
  stack.head_offset = 0;
  fixed_pool_init(&stack.pool, sizeof(int), initialSize);
  printf("stack_init exit\n");
  return stack;
}

int stack_peek(refstack* stack)
{
  return *(int*)fixed_pool_get(stack->pool,stack->head_offset); 
}

memref* stack_pop(refstack* stack)
{
  //  assert(stack->head_offset > 0);
  int* ref_off = (int*)fixed_pool_get(stack->pool,stack->head_offset);
  printf("popping stack with ref offset of %i",*ref_off);
  memref* ref = (memref*)fixed_pool_get(ref_memory,*ref_off);
  ref->refcount--;
  fixed_pool_free(stack->pool,stack->head_offset);
  stack->head_offset -= sizeof(int);
  return ref;
}

void stack_push(refstack* stack, memref* ref)//should this be a ptr??
{
  printf("stack_push enter\n");
  ref->refcount++;  
  int offset = fixed_pool_alloc(stack->pool);
  printf("new offset is %i\n",offset);
  int* ref_off = (int*)fixed_pool_get(stack->pool,offset);
  stack->head_offset = offset;
  *ref_off = ref->ref_off;
}

memref* malloc_ref(char type, int targ_offset)
{
  printf("malloc_ref entry\n");
  int ref_off = fixed_pool_alloc(ref_memory);
  printf("new ref offset is %i\n",ref_off);
  memref* ref = (memref*)fixed_pool_get(ref_memory,ref_off);
  ref->ref_off = ref_off;
  ref->targ_off = targ_offset;
  ref->type = type;
  ref->refcount = 0;
  printf("malloc_ref exit\n");
  return ref;
}

memref* malloc_int(int val)
{
  printf("malloc_int entry\n");
  //allocate int
  int int_off = fixed_pool_alloc(int_memory);
  int* int_val = (int*)fixed_pool_get(int_memory,int_off);
  *int_val = val;
  //alloc ref to it
  memref* ref = malloc_ref(Int32,int_off);
  printf("malloc_int exit\n");
  return ref;
}

memref* malloc_go()
{
  printf("malloc_go entry\n");
  int go_off = fixed_pool_alloc(go_memory);
  go* go_val = (go*)fixed_pool_get(go_memory,go_off);
  go_val->id = max_go_id++;
  go_val->kvp_count = 0;
  go_val->bucket_count = 3;
  go_val->buckets = 0;// todo: alloc dynamic space for kvp array
  memref* ref = malloc_ref(GameObject,go_off);
  printf("malloc_go exit\n");
  return ref;
}

void stack_push_int(refstack* stack, int value)
{
  stack_push(stack,malloc_int(42));
}


int memref_hash(memref* ref)
{
  unsigned hash = 42;
  if(ref->type == Int32)
    {
      return *(int*)fixed_pool_get(int_memory, ref->targ_off);
    }
  //todo
  
  return 42;
}

void debug_print_stack(refstack* stack)
{  
  int* ptr = (int*)fixed_pool_get(stack->pool,0);
  printf("stack head is at offset %i\n", stack->head_offset);
  int i = 0;
  do
    {
      printf("index %i ref offset %i\n",i,*ptr);
      memref* ref = (memref*)fixed_pool_get(ref_memory,*ptr);
      printf("\tmemref data:\n");
      printf("\t\ttype %i\n",ref->type);
      printf("\t\trefcount %i\n",ref->refcount);
      printf("\t\tref_off %i\n",ref->ref_off);
      printf("\t\ttarg_off %i\n",ref->targ_off);
      i += sizeof(int);
      ptr++;
    }
  while(i <= stack->head_offset);
}
    
    

int main()
{
  printf("entry\n");
  int memory = 0;
  
  fixed_pool_init(&int_memory,8,4);
  int x = fixed_pool_alloc(int_memory);
  int* y = (int*)fixed_pool_get(int_memory,x);
  *y = 42;
  

      
  /* fixed_pool_init(&ref_memory,sizeof(memref),1024); */
  /* fixed_pool_init(&go_memory,sizeof(go),1024); */
  /* dyn_pool_init(&dyn_memory,sizeof(kvp) * 1024); */

  /* printf("herehere"); */
  /* int off1 = dyn_pool_alloc_set(dyn_memory, 128, 0); */
  /* int off2 = dyn_pool_alloc_set(dyn_memory, 128, 1); */
  /* dyn_pool_free(dyn_memory, off1); */
  /* int  off3 = dyn_pool_alloc_set(dyn_memory, 64, 2); */
  /* int  off4 = dyn_pool_alloc_set(dyn_memory, 60, 3); */
  /* int  off5 = dyn_pool_alloc_set(dyn_memory, 64, 4); */
  /* dyn_pool_free(dyn_memory, off2); */
  /* int  off6 = dyn_pool_alloc_set(dyn_memory, 64, 5); */
  /* HANDLE hStdout, hStdin,hNewScreenBuffer;  */

 
  /*   // Get a handle to the STDOUT screen buffer to copy from and  */
  /*   // create a new screen buffer to copy to.  */

  /* hStdout = GetStdHandle(STD_OUTPUT_HANDLE); */
  /* hStdin = GetStdHandle(STD_INPUT_HANDLE);  */
  /* hNewScreenBuffer = CreateConsoleScreenBuffer(  */
  /*      GENERIC_READ |           // read/write access  */
  /*      GENERIC_WRITE,  */
  /*      FILE_SHARE_READ |  */
  /*      FILE_SHARE_WRITE,        // shared  */
  /*      NULL,                    // default security attributes  */
  /*      CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE  */
  /*      NULL);                   // reserved; must be NULL  */
  /*   if (hStdout == INVALID_HANDLE_VALUE ||  */
  /*           hNewScreenBuffer == INVALID_HANDLE_VALUE)  */
  /*   { */
  /*       printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());  */
  /*       return 1; */
  /*   } */

  /*   // Make the new screen buffer the active screen buffer.  */
  /*   if (! SetConsoleActiveScreenBuffer(hNewScreenBuffer) )  */
  /*   { */
  /*       printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());  */
  /*       return 1; */
  /*   } */

  /*   int count = 5; */
  /*   COORD cursorPosition; */
  /*   cursorPosition.Y = 25; */
  /*   cursorPosition.X = 0; */
  /*   char input[255]; */
  /*   char output[255]; */
  /*   for(int i = 0; i<255; i++) */
  /*     { */
  /*       input[i] = ' '; */
  /*       output[i] = ' '; */
  /*     } */
  /*   int read = 0; */
  /*   while(count > 0) */
  /*     { */
  /*       drawMemory(hNewScreenBuffer, memory); */
  /*       SetConsoleCursorPosition(hNewScreenBuffer, cursorPosition); */
  /*       WriteConsoleOutputCharacter(hNewScreenBuffer, output,255,cursorPosition, &read); */
  /*       ReadFile(hStdin, input, 255, &read, NULL); */
  /*       if(input[0] == 'a') */
  /*         { */
            
  /*           allocSet(atoi((char*)input+1),2,memory);             */
  /*         } */
  /*     } */

  /*   // Restore the original active screen buffer.  */

  /*   if (! SetConsoleActiveScreenBuffer(hStdout)) */
  /*   { */
  /*       printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError()); */
  /*       return 1; */
  /*   } */
  printf("exit");
  getchar();
}
