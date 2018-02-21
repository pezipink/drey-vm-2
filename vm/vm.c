#include "../memory/manager.h"
#include "../datastructs/refstack.h"
#include "../datastructs/refhash.h"
#include "../datastructs/refarray.h"
#include "vm.h"

#include<stdio.h>



unsigned char read_byte(vm * const vm, exec_context * const ec)
{
  unsigned char res = *(unsigned char*)ra_nth(vm->program,ec->pc++);
  return res;
}

unsigned short read_word(vm * const vm, exec_context * const ec)
{
  unsigned short res = read_byte(vm,ec) << 8;
  res |= read_byte(vm,ec);
  return res;
}

int read_int(vm* const vm, exec_context * const ec)
{
  unsigned int res = read_word(vm,ec) << 16;
  res |= read_word(vm,ec);
  return res;
}
 

int to_int(char* buf)
{
  int x = 0;
  x = *buf++;
  x <<= 8;
  x |= *buf++;
  x <<= 8;
  x |= *buf++;
  x <<= 8;
  x |= *buf;
  return x;
}

void read_program(vm* const vm)
{
  FILE *file;

  file=fopen("c:\\temp\\test.scur","rb");
  if (!file)
    {
      printf("Unable to open file!");
      return;
    }
  long lSize;
  DL("opened file\n");
  fseek(file,0,SEEK_END);
  lSize = ftell(file);
  rewind(file);
  //string table will start with an int indicating amount of strings
  //then each string prefixed with an int of how many chars
  char buffer[4];
  int totalStrings  = 0;
  fread(&buffer,sizeof(char),4,file);
  totalStrings = to_int(&buffer[0]);
  for(int i = 0; i < totalStrings; i++)
    {
      int len = 0;
      fread(&buffer,sizeof(char),4,file);
      len = to_int(&buffer[0]);
      lSize -= 4;
      lSize -= len * sizeof(char);
      memref r = ra_init(sizeof(char),len);
      r.type = String;
      ra_consume_capacity(r);
      refarray* ra = deref(&r);
      fread(&ra->data,sizeof(char),len,file);
      //      ra_wl(r);
      hash_set(vm->string_table,int_to_memref(i),r);
    }
  fread(&buffer,sizeof(char),4,file);
  vm->entry_point = to_int(&buffer[0]);
  vm->program = ra_init(sizeof(char),lSize);
  ra_consume_capacity(vm->program);
  refarray* ra = deref(&vm->program);
  fread(&ra->data,sizeof(char),lSize,file);
  
  fclose(file);
  DL("loaded program\n");
}

memref init_scope()
{
  unsigned int off = fixed_pool_alloc(scope_memory);
  scope* s = (scope*)fixed_pool_get(scope_memory, off);
  s->locals = hash_init(1);
  s->return_address = 0;
  memref r = malloc_ref(Scope,off);
  return r;
}

exec_context init_exec_context()
{
  exec_context ec;
  ec.eval_stack = stack_init(1);
  ec.pc = 0;
  memref scopes = ra_init(sizeof(memref),1);
  memref scope = init_scope();
  ra_append(scopes, &scope);
  ec.scopes = scopes;
  return ec;
}

fiber init_fiber(int id)
{
  fiber f;
  f.id = id;
  f.exec_contexts = ra_init(sizeof(exec_context),1);
  exec_context ex = init_exec_context();
  ra_append(f.exec_contexts,&ex);
  return f;
}

vm init_vm()
{
  vm v;
  v.string_table = hash_init(100);
  v.fibers = ra_init(sizeof(fiber),1);
  fiber f = init_fiber(0);
  ra_append(v.fibers,&f);
  read_program(&v);
  return v;
}


#define DIE(f_, ...) printf((f_), __VA_ARGS__); exit(0);
#define POP stack_pop(ec->eval_stack)
#define PEEK stack_peek(ec->eval_stack)
#define PEEK_REF stack_peek_ref(ec->eval_stack)
#define PUSH(V) stack_push(ec->eval_stack,V)
#define READ_INT  i =  read_int(vm,ec); 
#define READ_STR READ_INT \
      str = hash_get(vm->string_table,int_to_memref(i));

scope* current_scope(exec_context * const ec)
{
  int n = ra_count(ec->scopes) - 1;
  memref r = ra_nth_memref(ec->scopes,n);
  return (scope*)deref(&r);
}




memref load_var(scope* s, memref key)
{
  memref ret;
  
  if(hash_try_get(&ret,s->locals,key))
    {
      return ret;
    }
  else if(s->closure_scope.type == Scope)
    {
      return load_var(deref(&s->closure_scope),key);
    }
  else
    {
      DIE("!!!!!!!! CRITICAL ERROR, cOULD NOT FIND VAL\n");
    }

  memref r;
  r.type = 0;
  return r;
}

void replace_var(scope* s, memref key, memref val)
{
  if(hash_contains(s->locals,key))
    {
      hash_set(s->locals,key, val);
    }
  else if(s->closure_scope.type == Scope)
    {
      replace_var(deref(&s->closure_scope),key,val);
    }
  else
    {
      DIE("!!!!!!!! CRITICAL ERROR, COULD NOT FIND VAL\n");
    }
}


int step(vm* const vm, exec_context * const ec)
{
  enum opcode o = *(char*)ra_nth(vm->program,ec->pc++);
  int i;
  memref ma, mb, mc, md;
  memref* mp;
  memref str;
  scope* scope;
    switch(o)
    {
    case brk:
      VL("brk Not implemented\n");
      break;
      
    case pop:
      VL("pop\n");
      POP;
      break;
      
    case swap:
      VL("swap\n");
      ma = POP;
      mb = POP;
      PUSH(ma);
      PUSH(mb);
      break;
      
    case swapn:
      VL("swapn Not implemented\n");
      break;
      
    case dup:
      VL("dup Not implemented\n");
      break;
      
    case ldval:
      READ_INT;
      VL("ldval %i\n",i);
      PUSH(int_to_memref(i));
      break;
      
    case ldvals:
      READ_STR;
      #ifdef VM_DEBUG
      ra_wl(str);
      #endif
      PUSH(str);
      break;
      
    case ldvalb:
      VL("ldvalb Not implemented\n");
      break;
      
    case ldvar:
      VL("ldvar\n");
      READ_STR;
      #ifdef VM_DEBUG
      ra_wl(str);
      #endif
      ma = load_var(current_scope(ec),str);
      VL("loaded var ");
      #ifdef VM_DEBUG
      ra_w(str);
      #endif
      VL(" with a value of %i\n", ma.data.i);
         
      PUSH(ma);
      break;
      
    case stvar:
      VL("stvar ");
      READ_STR;
      #ifdef VM_DEBUG
      ra_wl(str);
      #endif
      scope = current_scope(ec);
      ma = POP;
      hash_set(scope->locals,str,ma);
      break;
      
    case p_stvar:
      VL("p_stvar ");
      READ_STR;
      #ifdef VM_DEBUG
      ra_wl(str);
      #endif
      scope = current_scope(ec);
      ma = PEEK;
      hash_set(scope->locals,str,ma);
      break;
      
    case rvar:
      VL("rvar ");      
      READ_STR;
#ifdef VM_DEBUG
      ra_wl(str);
      #endif
      replace_var(current_scope(ec),str,POP);
      break;
      
    case ldprop:
      VL("ldprop Not implemented\n");
      break;
      
    case p_ldprop:
      VL("p_ldprop Not implemented\n");
      break;
      
    case stprop:
      VL("stprop Not implemented\n");
      break;
      
    case p_stprop:
      VL("p_stprop Not implemented\n");
      break;
      
    case inc:
      VL("inc\n");
      mp = PEEK_REF;
      if(mp->type == Int32)
        {
          VL("data was %i\n", mp->data.i);
          mp->data.i ++;
          VL("now %i\n", mp->data.i);
        }
      else
        {
          DIE("!! inc expcted a int32 but got a %i\n",mp->type);
        }
      break;
      
    case dec:
      VL("dec\n");
      mp = PEEK_REF;
      if(mp->type == Int32)
        {
          mp->data.i--;
        }
      else
        {
          DIE("!! dec expcted a int32 but got a %i\n",mp->type);
        }
      break;
      
    case neg:
      VL("neg\n");
      mp = PEEK_REF;
      if(mp->type == Int32)
        {
          mp->data.i = -mp->data.i;
        }
      else
        {
          DIE("!! inc expcted a int32 but got a %i\n",mp->type);
        }
      break;
       
    case add:
      VL("add\n");
      ma = POP;
      mb = POP;
      //todo: support adding of strings
      if(ma.type != Int32 || mb.type != Int32)
        {
          DIE("add only supports ints!");
        }
      PUSH(int_to_memref( ma.data.i + mb.data.i));
      break;
    case sub:
      VL("sub\n");
      ma = POP;
      mb = POP;
      if(ma.type != Int32 || mb.type != Int32)
        {
          DIE("sub only supports ints!");
        }
      PUSH(int_to_memref( ma.data.i - mb.data.i)); 
      break;
    case mul:
      VL("mul\n");
      ma = POP;
      mb = POP;
      if(ma.type != Int32 || mb.type != Int32)
        {
          DIE("mul only supports ints!");
        }
      PUSH(int_to_memref( ma.data.i * mb.data.i)); 


      break;
    case _div:
      VL("div\n");
      ma = POP;
      mb = POP;
      if(ma.type != Int32 || mb.type != Int32)
        {
          DIE("div only supports ints!");
        }
      PUSH(int_to_memref( ma.data.i / mb.data.i)); 

      break;
    case mod:
      VL("mod\n");
      ma = POP;
      mb = POP;
      if(ma.type != Int32 || mb.type != Int32)
        {
          DIE("mod only supports ints!");
        }
      PUSH(int_to_memref( ma.data.i % mb.data.i)); 
      break;
    case _pow:
      VL("pow Not implemented\n");
      break;
    case tostr:
      VL("tostr Not implemented\n");
      break;
    case toint:
      VL("toint Not implemented\n");
      break;
    case rndi:
      VL("rndi Not implemented\n");
      break;
    case startswith:
      VL("startswith Not implemented\n");
      break;
    case p_startswith:
      VL("p_startswith Not implemented\n");
      break;
    case endswith:
      VL("endswith Not implemented\n");
      break;
    case p_endswith:
      VL("p_endswith Not implemented\n");
      break;
    case contains:
      VL("contains Not implemented\n");
      break;
    case p_contains:
      VL("p_contains Not implemented\n");
      break;
    case indexof:
      VL("indexof Not implemented\n");
      break;
    case p_indexof:
      VL("p_indexof Not implemented\n");
      break;
    case substring:
      VL("substring Not implemented\n");
      break;
    case p_substring:
      VL("p_substring Not implemented\n");
      break;

    case ceq:
      VL("ceq\n");
      PUSH(int_to_memref(memref_equal(POP,POP)));
      break;
      
    case cne:
      VL("cne\n");
      i = memref_equal(POP,POP);

      if(i == 0)
        {
          VL("cne was %i\n", 1);
          PUSH(int_to_memref(1));
        }
      else
        {
          VL("cne was %i\n", 0);
          PUSH(int_to_memref(0));
        }

    case cgt:
      break;
      VL("cgt Not implemented\n");
      break;
    case cgte:
      VL("cgte Not implemented\n");
      break;
    case clt:
      VL("clt Not implemented\n");
      break;
    case clte:
      VL("clte Not implemented\n");
      break;
    case beq:
      VL("beq\n");
      READ_INT;
      if(memref_equal(POP,POP))
        {
          ec->pc = i - 5;
        }

      break;
    case bne:
      VL("bne\n");
      READ_INT;
      ma = POP;
      mb = POP;
      if(!memref_equal(ma,mb))
        {
          ec->pc += i - 5;
        }
      break;
      
    case bgt:
      VL("bgt Not implemented\n");
      break;
      
    case blt:
      VL("blt Not implemented\n");
      break;
      
    case bt:
      VL("bt \n");
      READ_INT;
      ma = POP;
      if(ma.data.i != 0 )
        {
          VL("branch taken\n");
          ec->pc += i - 5;
        }
      break;
      
    case bf:
      VL("bf\n");
      READ_INT;
      ma = POP;
      if(ma.data.i == 0 )
        {
          VL("branch taken\n");
          ec->pc += i - 5;
        }
      break;
      
    case branch:
      VL("branch\n");
      READ_INT;
      ec->pc += i - 5;
      break;
      
    case isobj:
      VL("isobj Not implemented\n");
      break;
    case isint:
      VL("isint Not implemented\n");
      break;
    case isbool:
      VL("isbool Not implemented\n");
      break;
    case isloc:
      VL("isloc Not implemented\n");
      break;
    case islist:
      VL("islist Not implemented\n");
      break;
    case createobj:
      VL("createobj Not implemented\n");
      break;
    case cloneobj:
      VL("cloneobj Not implemented\n");
      break;
    case getobj:
      VL("getobj Not implemented\n");
      break;
    case getobjs:
      VL("getobjs Not implemented\n");
      break;
    case delprop:
      VL("delprop Not implemented\n");
      break;
    case p_delprop:
      VL("p_delprop Not implemented\n");
      break;
    case delobj:
      VL("delobj Not implemented\n");
      break;
    case moveobj:
      VL("moveobj Not implemented\n");
      break;
    case p_moveobj:
      VL("p_moveobj Not implemented\n");
      break;
    case createlist:
      VL("createlist\n");
      ma = ra_init(sizeof(memref),3);
      stack_push(ec->eval_stack,ma);
      break;
    case appendlist:
      VL("appendlist\n");
      ma = POP;
      mb = POP;
      #if DEBUG
      if(mb.type != Array)
        {
          VL("Execpted list in p_appendlist but got %i\n",mb.type);
        }
      #endif
      ra_append_memref(mb,ma);

      break;
    case p_appendlist:
      VL("p_appendlist\n");
      ma = POP;
      mb = PEEK;      
      #if DEBUG
      if(mb.type != Array)
        {
          VL("Execpted list in p_appendlist but got %i\n",mb.type);
        }
      #endif
      ra_append_memref(mb,ma);
      break;
    case prependlist:
      VL("prependlist Not implemented\n");
      break;
    case p_prependlist:
      VL("p_prependlist Not implemented\n");
      break;
    case removelist:
      VL("removelist Not implemented\n");
      break;
    case p_removelist:
      VL("p_removelist Not implemented\n");
      break;
    case len:
      VL("len\n");
      PUSH(int_to_memref(ra_count(POP)));
      break;
    case p_len:
      VL("p_len\n");
      PUSH(int_to_memref(ra_count(PEEK)));
      break;

    case index:
      VL("index\n");
      ma = POP;
      mb = POP;
      PUSH(ra_nth_memref(mb,ma.data.i));
      break;
      
    case p_index:
      VL("p_index\n");
      ma = POP;
      mb = PEEK;
      PUSH(ra_nth_memref(mb,ma.data.i));
      break;
      
    case setindex:
      VL("setindex\n");
      ma = POP;
      mb = POP;
      mc = POP;
      #if DEBUG
      if(mb.type != Int32)
        {
          DIE("setindex called with a non int index");
        }
      if(mc.type != Array)
        {
          DIE("setindex called with a non array ");
        }
      #endif
      ra_set_memref(mc,mb.data.i,ma);
      break;
      
    case keys:
      VL("keys Not implemented\n");
      break;
    case values:
      VL("values Not implemented\n");
      break;
    case syncprop:
      VL("syncprop Not implemented\n");
      break;
    case getloc:
      VL("getloc Not implemented\n");
      break;
    case genloc:
      VL("genloc Not implemented\n");
      break;
    case genlocref:
      VL("genlocref Not implemented\n");
      break;
    case setlocsibling:
      VL("setlocsibling Not implemented\n");
      break;
    case p_setlocsibling:
      VL("p_setlocsibling Not implemented\n");
      break;
    case setlocchild:
      VL("setlocchild Not implemented\n");
      break;
    case p_setlocchild:
      VL("p_setlocchild Not implemented\n");
      break;
    case setlocparent:
      VL("setlocparent Not implemented\n");
      break;
    case p_setlocparent:
      VL("p_setlocparent Not implemented\n");
      break;
    case getlocsiblings:
      VL("getlocsiblings Not implemented\n");
      break;
    case p_getlocsiblings:
      VL("p_getlocsiblings Not implemented\n");
      break;
    case getlocchildren:
      VL("getlocchildren Not implemented\n");
      break;
    case p_getlocchildren:
      VL("p_getlocchildren Not implemented\n");
      break;
    case getlocparent:
      VL("getlocparent Not implemented\n");
      break;
    case p_getlocparent:
      VL("p_getlocparent Not implemented\n");
      break;
    case setvis:
      VL("setvis Not implemented\n");
      break;
    case p_setvis:
      VL("p_setvis Not implemented\n");
      break;
    case adduni:
      VL("adduni Not implemented\n");
      break;
    case deluni:
      VL("deluni Not implemented\n");
      break;
    case splitat:
      VL("splitat Not implemented\n");
      break;
    case shuffle:
      VL("shuffle Not implemented\n");
      break;
    case sort:
      VL("sort Not implemented\n");
      break;
    case sortby:
      VL("sortby Not implemented\n");
      break;
    case genreq:
      VL("genreq Not implemented\n");
      break;
    case addaction:
      VL("addaction Not implemented\n");
      break;
    case p_addaction:
      VL("p_addaction Not implemented\n");
      break;
    case suspend:
      VL("suspend Not implemented\n");
      break;
    case cut:
      VL("cut Not implemented\n");
      break;
    case say:
      VL("say Not implemented\n");
      break;
    case pushscope:
      VL("pushscope Not implemented\n");
      break;
    case popscope:
      VL("popscope Not implemented\n");
      break;
    case lambda:
      VL("lambda Not implemented\n");
      break;
    case apply:
      VL("apply Not implemented\n");
      break;
    case ret:
      if(ra_count(ec->scopes) == 1)
        {
          VL("Program end.\n");          
          return 1;      
        }
      else
        {
          VL("ret: not implemented");
          break;
        }
      
    case dbg:      
      VL("dbg\n");
      ma = POP;
      if(ma.type==Int32)
        {
          printf("%i",ma.data.i);
        }
      else if(ma.type == String)
        {
          ra_w(ma);
        }
      else
        {
          printf("dbg not implemented for type %i\n", ma.type);
        }

      break;
    case dbgl:
      VL("dbgl\n");
      ma = stack_pop(ec->eval_stack);
      if(ma.type==Int32)
        {
          printf("%i\n",ma.data.i);
        }
      else if(ma.type == String)
        {
          ra_wl(ma);
        }
      else
        {
          printf("dbgl not implemented for type %i\n", ma.type);
        }
      break;

    }
  
  return 0;
}

void run(vm* vm)
{
  fiber* f = (fiber*)ra_nth(vm->fibers,0);
  exec_context* ec = (exec_context*)ra_nth(f->exec_contexts,0);
  while(step(vm,ec) == 0)
    {
      
    }

  VL("run finihsed\n");
}
