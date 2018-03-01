#ifndef _GLOBAL_H
#define _GLOBAL_H



//#define TRACE
//#define VM_DEBUG


#define DIE(f_, ...) printf((f_), __VA_ARGS__); exit(0);


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

enum MessageType
  {
    Connect = 0x1,
    Heartbeat = 0x2,
    Data  = 0x3,
    Status = 0x4,
    Universe = 0x5,
    Debug = 0x6
  };



#endif
