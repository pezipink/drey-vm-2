#include "debugger.h"
#include "vm.h"
#include "..\datastructs\refhash.h"
#include "..\datastructs\refarray.h"
#include "..\memory\manager.h"

static memref serialize_ref(memref ref, memref gos)
{
  memref ret = hash_init(3);
  switch(ref.type)
    {
    case Int32:
      hash_set(ret, ra_init_str("type"), ra_init_str("int"));
      hash_set(ret, ra_init_str("value"), int_to_memref(ref.data.i));            
      break;

      
    default:
      //printf("didn't know how to serialize type %i\n", ref.type);
      break;
    }
  return ret;
}


static memref serialize_scope(scope *s, memref gos)
{
  memref ret = hash_init(3);
  hash_set(ret, ra_init_str("is_fiber"), int_to_memref(s->is_fiber));
  hash_set(ret, ra_init_str("return_address"), int_to_memref(s->return_address));
  memref locals = hash_init(hash_count(s->locals));
  memref keys = hash_get_keys(s->locals);
  int size = ra_count(keys);
  for(int i = 0; i < size; i ++)
    {
      memref key = ra_nth_memref(keys,i);
      if(key.type != String)
        {
          // TODO: we can convert some types to strings.
          printf("couldnt not serialize scope local for type %i\n",key.type); 
        }
      else
        {
          hash_set(locals, key, serialize_ref(hash_get(s->locals, key), gos));
        }

    }
  hash_set(ret, ra_init_str("locals"), locals);
  return ret;
}

static memref serialize_exec_context(exec_context *ec, memref gos)
{
  memref ret = hash_init(3);
  hash_set(ret, ra_init_str("pc"), int_to_memref(ec->pc));
  int size = ra_count(ec->scopes);
  memref scopes = ra_init(sizeof(memref), size);
  for(int i = 0; i < size; i++)
    {
      memref scope_ref = ra_nth_memref(ec->scopes, i);
      scope *s = deref(&scope_ref);
      ra_append_memref(scopes, serialize_scope(s, gos));
    }
  hash_set(ret, ra_init_str("scopes"), scopes);
  return ret;
}

static memref serialize_fiber(fiber *fib, memref gos)
{
  memref ret = hash_init(3);
  hash_set(ret, ra_init_str("id"), int_to_memref(fib->id));
  int size = ra_count(fib->exec_contexts);
  memref contexts = ra_init(sizeof(memref), size);
  for(int i = 0; i < size; i++)
    {
      exec_context *ec = ra_nth(fib->exec_contexts, i);
      ra_append_memref(contexts, serialize_exec_context(ec, gos));
    }
  hash_set(ret, ra_init_str("exec_contexts"), contexts);
  return ret;
}

memref build_general_announce(vm *vm)
{
  memref root = hash_init(2);
  memref gos = hash_init(hash_count(vm->u_objs));  
  memref fibers = ra_init(sizeof(memref),ra_count(vm->fibers)); /// todo: support fibers first
  //serialize up the tree, collecting gameobjects on the way.
  int size = ra_count(vm->fibers);
  for(int i = 0; i < size; i++)
    {
      fiber* f = ra_nth(vm->fibers, i);
      ra_append_memref(fibers,serialize_fiber(f, gos));
    }
  hash_set(root, ra_init_str("type"), ra_init_str("announce"));
  hash_set(root, ra_init_str("fibers"), fibers);
  hash_set(root, ra_init_str("gameobjects"), gos);

  return root;
}

int handle_debug_msg(memref json, vm *vm)
{
  memref t = hash_try_get_string_key(json, "type");
  if(t.type != 0)
    {
      if(memref_cstr_equal(t, "get-program"))
        {
          int size = ra_count(vm->program);
          memref new_program = ra_init(sizeof(memref),size);
          for(int i = 0; i < size; i++)
            {
              byte* v = ra_nth(vm->program, i);
              ra_append_memref(new_program,int_to_memref(*v));
            }
          hash_set(json, ra_init_str("program"), new_program);

          memref new_strings = hash_init(hash_count(vm->string_table));
          memref keys = hash_get_keys(vm->string_table);
          size = ra_count(keys);
          for(int i = 0; i < size; i++)
            {
              //since json only allows strings as keys, we will
              //have to convert these numbers
              memref key = ra_nth_memref(keys,i);
              memref val = hash_get(vm->string_table, key);
              char buf[11];
              itoa(key.data.i, buf, 10);
              hash_set(new_strings, ra_init_str(buf), val);
            }
          
          hash_set(json, ra_init_str("strings"), new_strings);
          
          memref opcodes = hash_init(3);
          memref opcode;

          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(0));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("brk"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(1));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("pop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(2));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("swap"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(3));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("swapn"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(4));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("dup"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(5));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("ldval"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(6));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("ldvals"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(7));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("ldvalb"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(8));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("ldvar"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(9));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("stvar"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(10));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("p_stvar"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(11));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("rvar"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(12));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("ldprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(13));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_ldprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(14));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("stprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(15));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_stprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(16));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("inc"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(17));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("dec"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(18));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("neg"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(19));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("add"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(20));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("sub"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(21));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("mul"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(22));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("div"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(23));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("mod"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(24));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("pow"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(25));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("tostr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(26));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("toint"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(27));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("rndi"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(28));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("startswith"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(29));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_startswith"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(30));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("endswith"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(31));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_endswith"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(32));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("contains"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(33));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_contains"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(34));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("indexof"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(35));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_indexof"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(36));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("substring"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(37));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_substring"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(38));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("ceq"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(39));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("cne"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(40));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("cgt"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(41));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("cgte"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(42));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("clt"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(43));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("clte"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(44));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("beq"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(45));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("bne"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(46));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("bgt"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(47));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("blt"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(48));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("bt"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(49));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("bf"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(50));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("branch"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(51));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("isobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(52));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("isint"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(53));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("isbool"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(54));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("isloc"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(55));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("islist"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(56));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("createobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(57));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("cloneobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(58));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(59));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getobjs"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(60));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("delprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(61));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_delprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(62));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("delobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(63));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("moveobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(64));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_moveobj"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(65));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("createarr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(66));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("appendarr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(67));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_appendarr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(68));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("prependarr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(69));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_prependarr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(70));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("removearr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(71));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_removearr"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(72));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("len"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(73));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_len"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(74));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("index"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(75));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_index"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(76));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("setindex"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(77));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("keys"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(78));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("values"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(79));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("syncprop"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(80));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getloc"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(81));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("genloc"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(82));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("genlocref"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(83));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("setlocsibling"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(84));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_setlocsibling"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(85));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("setlocchild"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(86));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_setlocchild"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(87));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("setlocparent"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(88));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_setlocparent"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(89));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getlocsiblings"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(90));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_getlocsiblings"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(91));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getlocchildren"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(92));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_getlocchildren"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(93));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getlocparent"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(94));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_getlocparent"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(95));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("setvis"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(96));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_setvis"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(97));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("adduni"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(98));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("deluni"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(99));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("splitat"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(100));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("shuffle"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(101));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("sort"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(102));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("sortby"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(103));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("genreq"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(104));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("addaction"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(105));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("p_addaction"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(106));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("suspend"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(107));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("cut"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(108));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("say"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(109));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("pushscope"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(110));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("popscope"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(111));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(1));
          hash_set(opcodes, ra_init_str("lambda"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(112));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("apply"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(113));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("ret"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(114));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("dbg"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(115));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("dbgl"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(116));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("getraw"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(117));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("fork"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(118));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("join"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(119));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("cons"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(120));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("head"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(121));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("tail"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(122));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("islist"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(123));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("createlist"), opcode);
          opcode = hash_init(2);
          hash_set(opcode, ra_init_str("code"), int_to_memref(124));
          hash_set(opcode, ra_init_str("extended"), int_to_memref(0));
          hash_set(opcodes, ra_init_str("isfunc"), opcode);  
          
          hash_set(json, ra_init_str("opcodes"), opcodes);
          return 2;
        }
      else if(memref_cstr_equal(t, "set-breakpoint"))
        {

        }
      else if(memref_cstr_equal(t, "clear-breakpoint"))
        {

        }
      else if(memref_cstr_equal(t, "run"))
        {

        }
      else if(memref_cstr_equal(t, "step-into"))
        {

        }
    }
  else
    {
      printf("Couldnt find key 'type'\n");
    }
  return 0;
}



