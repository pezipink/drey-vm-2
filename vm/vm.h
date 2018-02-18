#ifndef _VM_H
#define _VM_H

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
		islist = 55,
		createobj = 56,
		cloneobj = 57,
		getobj = 58,
		getobjs = 59,
		delprop = 60,
		p_delprop = 61,
		delobj = 62,
		moveobj = 63,
		p_moveobj = 64,
		createlist = 65,
		appendlist = 66,
		p_appendlist = 67,
		prependlist = 68,
		p_prependlist = 69,
		removelist = 70,
		p_removelist = 71,
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
		dbgl = 115
	};
typedef struct scope
{
  memref locals; //obj->obj dict
  int return_address; //if a func
  memref closure_scope;
} scope;

typedef struct exec_context
{
  int pc;
  memref eval_stack;
  memref scopes; //ra memrefs
} exec_context;

typedef struct fiber
{
  int id;
  memref exec_contexts;
} fiber;

typedef struct vm
{
  int pc;
  memref program;
  memref string_table;
  int entry_point;
  memref fibers;
} vm;

vm init_vm();
#endif
