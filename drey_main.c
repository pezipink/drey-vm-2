#include <stdio.h>
#include <windows.h>
#include "memory\manager.h"
#include "datastructs\refhash.h"
#include "global.h"
#include <time.h>
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





    
    

int main()
{
  printf("entry\n");

  TL("test%i\n", 0);
  int memory = 0;
  
  fixed_pool_init(&int_memory,sizeof(int),1024);
  fixed_pool_init(&ref_memory,sizeof(memref),1024);
  fixed_pool_init(&hash_memory,sizeof(refhash),1024);
  fixed_pool_init(&kvp_memory,sizeof(key_value),2048);
  dyn_pool_init(&dyn_memory,sizeof(int) * 1024);

  memref* h = hash_init(0);
  memref* i = malloc_int(42);
  memref* j = malloc_int(58);

  hash_set(h,i,j);

  memref* r = hash_get(h, i);
  int* x = (int*)deref(r);
  TL("%i\n",*x);
  
  
/*   clock_t t; */
/*     t = clock(); */
/*         t = clock() - t; */
/*     double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds */
 
/*     printf("fun() took %f seconds to execute \n", time_taken); */
/* t = clock(); */

    /* for(int i =0; i < 10000000; i++) */
    /* { */
    /*   dyn_pool_alloc(dyn_memory, 1000000); */
    /*   //      fixed_pool_alloc(int_memory);  */
    /* } */
    /* t = clock() - t; */
    /*  time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds */
 
    /* printf("fun() took %f seconds to execute \n", time_taken); */

  /* int* y = (int*)fixed_pool_get(int_memory,x); */
  /* *y = 42; */
  

      
  /* fixed_pool_init(&ref_memory,sizeof(memref),1024); */
  /* fixed_pool_init(&go_memory,sizeof(go),1024); */
 

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
