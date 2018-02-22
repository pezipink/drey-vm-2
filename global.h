#ifndef _GLOBAL_H
#define _GLOBAL_H



//#define TRACE
#define VM_DEBUG

#ifdef TRACE
#define TL(f_, ...) printf((f_), __VA_ARGS__)
#else
#define TL(F,...)
#endif

#ifdef DEBUG
#define DL(f_, ...) printf((f_), __VA_ARGS__)
#else
#define DL(F,...)
#endif

#ifdef VM_DEBUG
#define VL(f_, ...) printf((f_), __VA_ARGS__)
#else
#define VL(F,...)
#endif


#define true 1
#define false 0
#define bool int

#endif
