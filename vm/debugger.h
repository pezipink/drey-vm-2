#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "vm.h"
#include <windows.h>


int handle_debug_msg(memref msg, vm *vm);
memref build_general_announce(vm *vm);

#endif
