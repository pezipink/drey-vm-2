#ifndef _VM_H
#define _VM_H
#include <winsock2.h>
#include <windows.h>

#include "..\memory\manager.h"
#include "..\zmq.h"
enum opcode
	{
		brk = 0,
		pop = 1,
		swap = 2,
		swapn = 3,
		dup = 4,
		ldval = 5,
		ldvals = 6,
		ldvalb = 7,
		ldvar = 8,
		stvar = 9,
		p_stvar = 10,
		rvar = 11,
		ldprop = 12,
		p_ldprop = 13,
		stprop = 14,
		p_stprop = 15,
		inc = 16,
		dec = 17,
		neg = 18,
		add = 19,
		sub = 20,
		mul = 21,
		_div = 22,
		mod = 23,
		_pow = 24,
		tostr = 25,
		toint = 26,
		rndi = 27,
		startswith = 28,
		p_startswith = 29,
		endswith = 30,
		p_endswith = 31,
		contains = 32,
		p_contains = 33,
		indexof = 34,
		p_indexof = 35,
		substring = 36,
		p_substring = 37,
		ceq = 38,
		cne = 39,
		cgt = 40,
		cgte = 41,
		clt = 42,
		clte = 43,
		beq = 44,
		bne = 45,
		bgt = 46,
		blt = 47,
		bt = 48,
		bf = 49,
		branch = 50,
		isobj = 51,
		isint = 52,
		isbool = 53,
		isloc = 54,
		isarray = 55,
		createobj = 56,
		cloneobj = 57,
		getobj = 58,
		getobjs = 59,
		delprop = 60,
		p_delprop = 61,
		delobj = 62,
		moveobj = 63,
		p_moveobj = 64,
		createarr = 65,
		appendarr = 66,
		p_appendarr = 67,
		prependarr = 68,
		p_prependarr = 69,
		removearr = 70,
		p_removearr = 71,
		len = 72,
		p_len = 73,
		index = 74,
		p_index = 75,
		setindex = 76,
		keys = 77,
		values = 78,
		syncprop = 79,
		getloc = 80,
		genloc = 81,
		genlocref = 82,
		setlocsibling = 83,
		p_setlocsibling = 84,
		setlocchild = 85,
		p_setlocchild = 86,
		setlocparent = 87,
		p_setlocparent = 88,
		getlocsiblings = 89,
		p_getlocsiblings = 90,
		getlocchildren = 91,
		p_getlocchildren = 92,
		getlocparent = 93,
		p_getlocparent = 94,
		setvis = 95,
		p_setvis = 96,
		adduni = 97,
		deluni = 98,
		splitat = 99,
		shuffle = 100,
		sort = 101,
		sortby = 102,
		genreq = 103,
		addaction = 104,
		p_addaction = 105,
		suspend = 106,
		cut = 107,
		say = 108,
		pushscope = 109,
		popscope = 110,
		lambda = 111,
		apply = 112,
		ret = 113,
		dbg = 114,
		dbgl = 115,
                getraw = 116,
                fork = 117,
                join = 118,
                cons = 119,
                head = 120,
                tail = 121,
                islist = 122,
                createlist = 123,
                isfunc = 124
                
	};


typedef struct exec_context
{
  int id;
  int pc;
  memref eval_stack;
  raref scopes; //ra memrefs
} exec_context;

enum response_types
  {
    None = 0,
    Choice = 1,
    RawData = 2,
    Join = 3
  };
  

typedef struct fiber
{
  int id;
  raref exec_contexts;
  
  enum response_types awaiting_response;
  // details of message sent to client
  // so it can be resent and response validated.
  
  stringref waiting_client;
  hashref waiting_data;
  
} fiber;

typedef struct vm
{
  int pc;
  int gc_off;
  int cycle_count;
  raref  program;
  hashref string_table;
  int entry_point;
  raref fibers;

  int req_players;
  int num_players;
  
  int game_over;
  //universe
  int u_max_id;
  hashref u_objs;
  //hashref u_locrefs;
  hashref u_locs;

  //zmq socket.
  void* ip_machine;

  //DEBUG
  int exec_fiber_id;
  int exec_context_id;
  int debugger_connected;
} vm;

vm init_vm(void* socket);
zmq_msg_t ra_to_msg(memref ra);
void vm_client_connect(vm* machine, char* clientid, int len);

void vm_handle_response(vm* machine, stringref clientid, memref choice);
void vm_handle_raw(vm* machine, char* clientid, int clientLen, char* response, int responseLen);

void vm_run(vm *vm);

#endif

