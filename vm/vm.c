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

scope* current_scope(exec_context * const ec)
{
  int n = ra_count(ec->scopes) - 1;
  memref r = ra_nth_memref(ec->scopes,n);
  return (scope*)deref(&r);
}

memref load_val(scope* s, memref key)
{
  if(hash_contains(s->locals,key))
    {
      return hash_get(s->locals,key);
    }
  if(s->closure_scope.type == Scope)
    {
      return load_val(deref(&s->closure_scope),key);
    }
  else
    {
      DL("!!!!!!!! CRITICAL ERROR, cOULD NOT FIND VAL\n");

    }     
}

int step(vm* const vm, exec_context * const ec)
{
  enum opcode o = *(char*)ra_nth(vm->program,ec->pc++);
  int val;
  
  memref str;
    switch(o)
    {
    case brk:
      DL("brk Not implemented\n");
      break;
    case pop:
      DL("pop Not implemented\n");
      break;
    case swap:
      DL("swap Not implemented\n");
      break;
    case swapn:
      DL("swapn Not implemented\n");
      break;
    case dup:
      DL("dup Not implemented\n");
      break;
    case ldval:
      val =  read_int(vm,ec);
      DL("ldval %i\n",val);
      stack_push(ec->eval_stack,int_to_memref(val));
      break;
    case ldvals:
      val =  read_int(vm,ec); // string index
      DL("sldvals %i : ",val);
      str =hash_get(vm->string_table,int_to_memref(val));
      ra_wl(str);
      stack_push(ec->eval_stack,str);
      break;
    case ldvalb:
      DL("ldvalb Not implemented\n");
      break;
    case ldvar:
      val =  read_int(vm,ec); // string index
      DL("ldvar %i : ",val);
      str=hash_get(vm->string_table,int_to_memref(val));
      stack_push(ec->eval_stack,load_val(current_scope(ec),str));
      break;
    case stvar:
      val =  read_int(vm,ec); // string index
      DL("stvar %i : ",val);
      str =hash_get(vm->string_table,int_to_memref(val));
      ra_wl(str);
      scope* scope = current_scope(ec);
      memref val = stack_pop(ec->eval_stack);
      hash_set(scope->locals,str,val);
      break;
    case p_stvar:
      DL("p_stvar Not implemented\n");
      break;
    case rvar:
      DL("rvar Not implemented\n");
      break;
    case ldprop:
      DL("ldprop Not implemented\n");
      break;
    case p_ldprop:
      DL("p_ldprop Not implemented\n");
      break;
    case stprop:
      DL("stprop Not implemented\n");
      break;
    case p_stprop:
      DL("p_stprop Not implemented\n");
      break;
    case inc:
      DL("inc Not implemented\n");
      break;
    case dec:
      DL("dec Not implemented\n");
      break;
    case neg:
      DL("neg Not implemented\n");
      break;
    case add:
      DL("add\n");
      memref a = stack_pop(ec->eval_stack);
      memref b = stack_pop(ec->eval_stack);
      int c = a.data.i + b.data.i;
      stack_push(ec->eval_stack,int_to_memref(c));
      break;
    case sub:
      DL("sub Not implemented\n");
      break;
    case mul:
      DL("mul Not implemented\n");
      break;
    case _div:
      DL("div Not implemented\n");
      break;
    case mod:
      DL("mod Not implemented\n");
      break;
    case _pow:
      DL("pow Not implemented\n");
      break;
    case tostr:
      DL("tostr Not implemented\n");
      break;
    case toint:
      DL("toint Not implemented\n");
      break;
    case rndi:
      DL("rndi Not implemented\n");
      break;
    case startswith:
      DL("startswith Not implemented\n");
      break;
    case p_startswith:
      DL("p_startswith Not implemented\n");
      break;
    case endswith:
      DL("endswith Not implemented\n");
      break;
    case p_endswith:
      DL("p_endswith Not implemented\n");
      break;
    case contains:
      DL("contains Not implemented\n");
      break;
    case p_contains:
      DL("p_contains Not implemented\n");
      break;
    case indexof:
      DL("indexof Not implemented\n");
      break;
    case p_indexof:
      DL("p_indexof Not implemented\n");
      break;
    case substring:
      DL("substring Not implemented\n");
      break;
    case p_substring:
      DL("p_substring Not implemented\n");
      break;
    case ceq:
      DL("ceq Not implemented\n");
      break;
    case cne:
      DL("cne Not implemented\n");
      break;
    case cgt:
      DL("cgt Not implemented\n");
      break;
    case cgte:
      DL("cgte Not implemented\n");
      break;
    case clt:
      DL("clt Not implemented\n");
      break;
    case clte:
      DL("clte Not implemented\n");
      break;
    case beq:
      DL("beq Not implemented\n");
      break;
    case bne:
      DL("bne Not implemented\n");
      break;
    case bgt:
      DL("bgt Not implemented\n");
      break;
    case blt:
      DL("blt Not implemented\n");
      break;
    case bt:
      DL("bt Not implemented\n");
      break;
    case bf:
      DL("bf Not implemented\n");
      break;
    case branch:
      DL("branch Not implemented\n");
      break;
    case isobj:
      DL("isobj Not implemented\n");
      break;
    case isint:
      DL("isint Not implemented\n");
      break;
    case isbool:
      DL("isbool Not implemented\n");
      break;
    case isloc:
      DL("isloc Not implemented\n");
      break;
    case islist:
      DL("islist Not implemented\n");
      break;
    case createobj:
      DL("createobj Not implemented\n");
      break;
    case cloneobj:
      DL("cloneobj Not implemented\n");
      break;
    case getobj:
      DL("getobj Not implemented\n");
      break;
    case getobjs:
      DL("getobjs Not implemented\n");
      break;
    case delprop:
      DL("delprop Not implemented\n");
      break;
    case p_delprop:
      DL("p_delprop Not implemented\n");
      break;
    case delobj:
      DL("delobj Not implemented\n");
      break;
    case moveobj:
      DL("moveobj Not implemented\n");
      break;
    case p_moveobj:
      DL("p_moveobj Not implemented\n");
      break;
    case createlist:
      DL("createlist Not implemented\n");
      break;
    case appendlist:
      DL("appendlist Not implemented\n");
      break;
    case p_appendlist:
      DL("p_appendlist Not implemented\n");
      break;
    case prependlist:
      DL("prependlist Not implemented\n");
      break;
    case p_prependlist:
      DL("p_prependlist Not implemented\n");
      break;
    case removelist:
      DL("removelist Not implemented\n");
      break;
    case p_removelist:
      DL("p_removelist Not implemented\n");
      break;
    case len:
      DL("len Not implemented\n");
      break;
    case p_len:
      DL("p_len Not implemented\n");
      break;
    case index:
      DL("index Not implemented\n");
      break;
    case p_index:
      DL("p_index Not implemented\n");
      break;
    case setindex:
      DL("setindex Not implemented\n");
      break;
    case keys:
      DL("keys Not implemented\n");
      break;
    case values:
      DL("values Not implemented\n");
      break;
    case syncprop:
      DL("syncprop Not implemented\n");
      break;
    case getloc:
      DL("getloc Not implemented\n");
      break;
    case genloc:
      DL("genloc Not implemented\n");
      break;
    case genlocref:
      DL("genlocref Not implemented\n");
      break;
    case setlocsibling:
      DL("setlocsibling Not implemented\n");
      break;
    case p_setlocsibling:
      DL("p_setlocsibling Not implemented\n");
      break;
    case setlocchild:
      DL("setlocchild Not implemented\n");
      break;
    case p_setlocchild:
      DL("p_setlocchild Not implemented\n");
      break;
    case setlocparent:
      DL("setlocparent Not implemented\n");
      break;
    case p_setlocparent:
      DL("p_setlocparent Not implemented\n");
      break;
    case getlocsiblings:
      DL("getlocsiblings Not implemented\n");
      break;
    case p_getlocsiblings:
      DL("p_getlocsiblings Not implemented\n");
      break;
    case getlocchildren:
      DL("getlocchildren Not implemented\n");
      break;
    case p_getlocchildren:
      DL("p_getlocchildren Not implemented\n");
      break;
    case getlocparent:
      DL("getlocparent Not implemented\n");
      break;
    case p_getlocparent:
      DL("p_getlocparent Not implemented\n");
      break;
    case setvis:
      DL("setvis Not implemented\n");
      break;
    case p_setvis:
      DL("p_setvis Not implemented\n");
      break;
    case adduni:
      DL("adduni Not implemented\n");
      break;
    case deluni:
      DL("deluni Not implemented\n");
      break;
    case splitat:
      DL("splitat Not implemented\n");
      break;
    case shuffle:
      DL("shuffle Not implemented\n");
      break;
    case sort:
      DL("sort Not implemented\n");
      break;
    case sortby:
      DL("sortby Not implemented\n");
      break;
    case genreq:
      DL("genreq Not implemented\n");
      break;
    case addaction:
      DL("addaction Not implemented\n");
      break;
    case p_addaction:
      DL("p_addaction Not implemented\n");
      break;
    case suspend:
      DL("suspend Not implemented\n");
      break;
    case cut:
      DL("cut Not implemented\n");
      break;
    case say:
      DL("say Not implemented\n");
      break;
    case pushscope:
      DL("pushscope Not implemented\n");
      break;
    case popscope:
      DL("popscope Not implemented\n");
      break;
    case lambda:
      DL("lambda Not implemented\n");
      break;
    case apply:
      DL("apply Not implemented\n");
      break;
    case ret:
      if(ra_count(ec->scopes) == 1)
        {
          DL("Program end.\n");          
          return 1;      
        }
      else
        {
          DL("ret: not implemented");
          break;
        }
      
    case dbg:
      DL("dbg Not implemented\n");
      break;
    case dbgl:
      DL("dbgl\n");
      memref r = stack_pop(ec->eval_stack);
      if(r.type==Int32)
        {
          DL("%i\n",r.data.i);
        }
      else if(r.type == String)
        {
          ra_wl(r);
        }
      else
        {
          DL("dbgl not implemented for type %i\n", r.type);
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

  DL("run finihsed\n");
}
