#include <winsock2.h>
#include <windows.h>

#include "../memory/manager.h"
#include "../datastructs/refstack.h"
#include "../datastructs/refhash.h"
#include "../datastructs/refarray.h"
#include "../datastructs/reflist.h"
#include "vm.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>

void print_stack(exec_context* ec)
{
  refstack* rs = deref(&ec->eval_stack);
  int offset = rs->head_offset;
  memref* head = stack_peek_ref(ec->eval_stack);
  while(offset >= 0)
    {
      printf("%i : %i | ", offset, head->type);
      head--;
      offset -= sizeof(memref);
    }
  printf("\n");
}

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
 

unsigned int to_int(unsigned char* buf)
{
  unsigned int x = 0;
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
  unsigned char buffer[4];
  unsigned int totalStrings  = 0;
  fread(&buffer,sizeof(char),4,file);
  totalStrings = to_int(&buffer[0]);
  DL("total string table count is %i\n", totalStrings);
  for(int i = 0; i < totalStrings; i++)
    {
      int len = 0;
      fread(&buffer,sizeof(char),4,file);
      len = to_int(&buffer[0]);
      lSize -= 4;
      lSize -= len * sizeof(char);
      memref r = ra_init_raw(sizeof(char),len, String);

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
  gc_print_stats();
}

memref init_scope()
{
  unsigned int off = fixed_pool_alloc(scope_memory);
  scope* s = (scope*)fixed_pool_get(scope_memory, off);
  s->locals = hash_init(1);
  s->return_address = 0;
  s->is_fiber = 0;
  memref r = malloc_ref(Scope,off);  
  return r;
}

void patch_stack_pool_pointers(vm* vm)
{
  int fm = ra_count(vm->fibers);
  for(int fi = 0; fi < fm; fi++)
    {
      fiber* f = ra_nth(vm->fibers, fi);
      int em = ra_count(f->exec_contexts);
      for(int ei = 0; ei < em; ei++)
        {
          exec_context* ec = ra_nth(f->exec_contexts, ei);
          refstack* rs = deref(&ec->eval_stack);
          rs->pool->owner = &rs->pool;
        }
    }
}

exec_context init_exec_context()
{
  exec_context ec;
  ec.eval_stack = stack_init(1);

  ec.pc = 0;
  memref scopes = ra_init(sizeof(memref),1);
  memref scopeRef = init_scope();
  scope* s = deref(&scopeRef);
  ra_append_memref(scopes, scopeRef);
  ec.scopes = scopes;
  return ec;
}

fiber init_fiber(int id)
{
  fiber f;
  f.id = id;
  f.exec_contexts = ra_init(sizeof(exec_context),1);
  f.awaiting_response = None;
  f.waiting_client.type = 0;
  f.waiting_data.type = 0;
  f.valid_responses = hash_init(3);
  exec_context ex = init_exec_context();
  ra_append(f.exec_contexts,&ex);
  return f;
}

vm init_vm(void* socket)
{
  printf("1 reference pool at %p\n",ref_memory);
  vm v;
  
  time_t t;
  srand((unsigned) time(&t));
  
  v.ip_machine = socket;
  v.string_table = hash_init(100);
  
  v.fibers = ra_init(sizeof(fiber),1);

  v.game_over = false;
  
  v.gc_off = 0;
  v.cycle_count = 0;

  v.num_players = 0;
  v.req_players = 1; //todo: load this somehow
  
  fiber f = init_fiber(0);
  ra_append(v.fibers,&f);

  f.valid_responses.type = 0;
  f.waiting_client.type = 0;
  f.waiting_data.type = 0;
  
  //universe
  v.u_max_id = 0;
  v.u_objs = hash_init(3);
  //  v.u_locrefs = hash_init(3);
  v.u_locs = hash_init(3);

  //global state object
  memref state = malloc_go(-1);  
  hashref players = hash_init(3);
  gameobject* go = deref(&state);
  hash_set(go->props, ra_init_str("players"), players);
  hash_set(v.u_objs,int_to_memref(-1),state);
  read_program(&v);
  return v;
}

zmq_msg_t ra_to_msg(memref ra)
{
  int len = ra_count(ra);
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, len);
  memcpy(zmq_msg_data(&msg), ra_data(ra), len);
  return msg;
}

exec_context* current_ec(fiber * const f)
{
  int n = ra_count(f->exec_contexts) - 1;
  return (exec_context*)ra_nth(f->exec_contexts,n);
}

fiber* get_fiber(vm* const vm, int index)
{
  return (fiber*)ra_nth(vm->fibers,index);
}

exec_context* get_fiber_ec(vm* const vm, int index)
{
  fiber*f = get_fiber(vm, index);
  return current_ec(f);
}


exec_context clone_current_exec_context(vm* vm, int fiber_index)
{
  exec_context new;
  exec_context* old = get_fiber_ec(vm, fiber_index);
  new.pc = old->pc;
  new.eval_stack = stack_clone(old->eval_stack);
  old = get_fiber_ec(vm, fiber_index);
  new.scopes = ra_clone(old->scopes);
  return new;
}

void vm_handle_raw(vm* vm, char* clientid, int clientLen, char* response, int responseLen)
{
  if(vm->game_over)
    {
      return;
    }
  memref id = ra_init_str_raw(clientid,clientLen);
  memref state_r = hash_get(vm->u_objs,int_to_memref(-1));
  memref choice = ra_init_str_raw(response,responseLen);
  gameobject* state_go = deref(&state_r);
  memref players = hash_get(state_go->props, ra_init_str("players"));

  if(hash_contains(players, id))
    {
      //todo: the specific fiber will be included in the message somehow.
      //for now we simply look for any fiber that is waiting for this client.
      int count = ra_count(vm->fibers);
      for(int i = 0; i < count; i++)
        {
          fiber* f = ra_nth(vm->fibers, i);
          if(f->awaiting_response == RawData && memref_equal(f->waiting_client, id))
            {
              //unload the entire data into a "string"
              memref data = ra_init_str_raw(response, responseLen);
              f = ra_nth(vm->fibers, i);
              f->awaiting_response = false;
              exec_context* ec = current_ec(f);
              stack_push(ec->eval_stack, data);
              return;
            }
        }
    }
  else
    {
      printf("response recieved from unknown player\n");
    }
}

void vm_handle_response(vm* vm, char* clientid, int clientLen, char* response, int responseLen)
{
  if(vm->game_over)
    {
      return;
    }
  memref id = ra_init_str_raw(clientid,clientLen);
  memref state_r = hash_get(vm->u_objs,int_to_memref(-1));
  memref choice = ra_init_str_raw(response,responseLen);
  gameobject* state_go = deref(&state_r);
  memref players = hash_get(state_go->props, ra_init_str("players"));

  if(hash_contains(players, id))
    {
      //todo: the specific fiber will be included in the message somehow.
      //for now we simply look for any fiber that is waiting for this client.
      int count = ra_count(vm->fibers);
      for(int i = 0; i < count; i++)
        {
          fiber* f = ra_nth(vm->fibers, i);
          if(f->awaiting_response == Choice && memref_equal(f->waiting_client, id))
            {              
              //check this is a valid response              
              if(hash_contains(f->valid_responses, choice))
                {
                  f->awaiting_response = false;
                  if(memref_equal(choice,ra_init_str("__UNDO__")))
                    {
                      printf("falling back to previous stack\n");
                      refarray* ra = deref(&f->exec_contexts);
                      printf("stack count was %i\n", ra->element_count);
                      ra->element_count-=2;
                      exec_context* ec = current_ec(f);
                      ec->pc--;
                    }
                  else
                    {
                      exec_context* ec = current_ec(f);
                      printf("pushing choice onto fiber %i stack\n", i);
                      stack_push(ec->eval_stack, choice);
                    }
                  return;
                }
              else
                {
                  printf("received an invalid response\n");
                }
                 
              return;
            }
        }
    }
  else
    {
      printf("response recieved from unknown player\n");
    }
}

void vm_client_connect(vm* vm, char* clientid, int len)
{
  memref id = ra_init_str_raw(clientid,len);
  memref state_r = hash_get(vm->u_objs,int_to_memref(-1));
  gameobject* state_go = deref(&state_r);
  memref players = hash_get(state_go->props, ra_init_str("players"));       
  if(hash_contains(players, id))
    {
      //player reconnected. send them a request message if
      //a fiber is waiting on them.
      int count = ra_count(vm->fibers);
      for(int i = 0; i < count; i++)
        {
          fiber* f = ra_nth(vm->fibers, i);
          //todo: handle other response types
          if(f->awaiting_response == Choice && memref_equal(f->waiting_client, id))
            {
              zmq_msg_t msg_id = ra_to_msg(id);
              zmq_msg_t msg_type;
              zmq_msg_init_size(&msg_type,1);
              char* data = zmq_msg_data(&msg_type); 
              *data = (char)Data;
              zmq_msg_t msg_data = ra_to_msg(f->waiting_data);
              zmq_msg_send(&msg_id, vm->ip_machine, ZMQ_SNDMORE);
              zmq_msg_send(&msg_type, vm->ip_machine, ZMQ_SNDMORE);
              zmq_msg_send(&msg_data, vm->ip_machine, 0);      
            }
        }
    }
  else
    {
     
      if(vm->num_players == vm->req_players)
        {
          printf("maximum players already connected\n");
        }
      else
        {
          memref go_r = malloc_go(vm->u_max_id++);
          gameobject* go = deref(&go_r);
          hash_set(go->props,ra_init_str("clientid"),id);      
          hash_set(players, id, go_r);
      
          printf("client ");
          ra_w(id);
          printf("is now in the game!\n");
          vm->num_players++;
          printf("waiting for %i more players.\n", vm->req_players - vm->num_players);        
      
        }
    }
}


#define POP stack_pop(ec->eval_stack)
#define PEEK stack_peek(ec->eval_stack)
#define PEEK_REF stack_peek_ref(ec->eval_stack)
#define PUSH(V) stack_push(ec->eval_stack,V)
#define READ_INT  i =  read_int(vm,ec); 
#define READ_STR READ_INT \
      str = hash_get(vm->string_table,int_to_memref(i));
#define REFRESH_EC  ec = get_fiber_ec(vm, fiber_index);


    
scope* current_scope(exec_context * const ec)
{
  int n = ra_count(ec->scopes) - 1;
  memref r = ra_nth_memref(ec->scopes,n);
  return (scope*)deref(&r);
}

memref current_scope_ref(exec_context * const ec)
{
  int n = ra_count(ec->scopes) - 1;
  memref r = ra_nth_memref(ec->scopes,n);
  return r;
}


memref init_function(exec_context * const ec, int location)
{
  unsigned int off = fixed_pool_alloc(func_memory);
  
  function* f = (function*)fixed_pool_get(func_memory, off); 
  
  memref s = current_scope_ref(ec);
  f->closure_scope = s;

  f->address = ec->pc + location - 5;  
  memref r = malloc_ref(Function,off);
  return r;
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
      printf("!!!!!!!! CRITICAL ERROR, cOULD NOT FIND VAL ");
      ra_w(key);
      DIE("!");
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

memref load_prop(memref key, memref obj)
{
  switch(obj.type)
    {
    case GameObject:
      gameobject *g = deref(&obj);
      return hash_get(g->props,key);
    case Hash:
      return hash_get(obj,key);
    case Location:
      location *l = deref(&obj);
      return hash_get(l->props,key);
    case LocationReference:
      locationref *lr = deref(&obj);
      return hash_get(lr->props,key);
    default:
      printf("invalid type %i\n", obj.type);
      break;
    }

  DIE("could not load prop\n");
  memref r;
  r.type = 0;
  return r;
}

void store_prop(vm* vm, memref val, memref key, memref obj)
{
  switch(obj.type)
    {
    case GameObject:
      gameobject* g = deref(&obj);
      hash_set(g->props,key,val);
      return;
    case Location:
      location *l = deref(&obj);
      hash_set(l->props,key,val);
      return;
    case LocationReference:
      locationref *lr = deref(&obj);
      hash_set(lr->props,key,val);
      return;
    default:
      printf("invalid type %i\n", obj.type);
      break;

    }

  DIE("could not set prop\n");
}


inline bool ref_contains(memref obj, memref key)
{
  switch(obj.type)
    {
    case GameObject:
      go* g = deref(&obj);
      return hash_contains(g->props,key);
      break;
      
    default:
      DIE("contains not implemented for type %i\n", obj.type);

    }
  return 0;
}

inline memref unpack_location(vm *const vm, memref source)
{

  if(source.type == String)
    {
      memref out = nullref();
      if(!hash_try_get(&out, vm->u_locs, source))
        {
          DIE("location key did not exist in universe!");
        }
      return out;
    }
  else if(source.type == Location)
    {
      return source;
    }
  else if(source.type == LocationReference)
    {
      locationref *lrp;
      lrp = deref(&source);
      memref out = nullref();
      if(!hash_try_get(&out, vm->u_locs, lrp->target_key))
        {
          DIE("location key did not exist in universe!");
        }
      return out;      
    }

  DIE("! source type was %i in unpack location\n", source.type );
  return nullref();
}

void get_nested_gameobjects(memref ra_out, memref to_search)
{
  // retrieve nested game objects - we need to check properties of game objects
  // arrays and hashes (TODO: probably the new lists too)
  if(to_search.type == GameObject)
    {
      ra_append_memref(ra_out, to_search);
      gameobject* go = deref(&to_search);
      get_nested_gameobjects(ra_out, go->props);
    }
  else if(to_search.type == Array)
    {
      int max = ra_count(to_search);
      for(int i = 0; i < max; i++)
        {
          get_nested_gameobjects(ra_out, ra_nth_memref(to_search,i));
        }
    }
  else if(to_search.type == Hash)
    {
      get_nested_gameobjects(ra_out, hash_get_values(to_search));
    }      
}

void move_gameobject(vm *const vm, memref go, memref loc)
{
  assert(go.type == GameObject);
  assert(loc.type == Location);
  gameobject *gop;
  location *locp;
  gop = deref(&go);
  locp = deref(&loc);
  // moving to a location always causes an object to exist in the universe   
  if(!hash_contains(vm->u_objs,int_to_memref(gop->id)))
    {
      DL("adding %i to uiniverse..\n", gop->id);
      hash_set(vm->u_objs, int_to_memref(gop->id), go);
      //TODO: Announce delta
    }

  // remove the object from its current location where applicable
  if(gop->location_key.type == String && !memref_equal(gop->location_key,locp->key))
    {
      memref current = hash_get(vm->u_locs, gop->location_key);
      location *lc = deref(&current);
      hash_remove(lc->objects, int_to_memref(gop->id));
    }

  if(loc.type == 0 )
    {
      //TODO: handle this case. should we remove from universe here?
      DIE("not implemented moving game object to null");
    }
  
  // add object to the new location
  gop->location_key = locp->key;
  hash_set(locp->objects, int_to_memref(gop->id), go);
  
  memref more = ra_init(sizeof(memref), 3);
  get_nested_gameobjects(more, go);
  int max = ra_count(more);
  gameobject *gop2;
  for(int i = 0; i < max; i++)
    {
      memref current = ra_nth_memref(more, i);
      gop2 = deref(&current);
      if(!hash_contains(vm->u_objs,int_to_memref(gop2->id)))
        {
          DL("adding %i to uiniverse..\n", gop2->id);
          hash_set(vm->u_objs, int_to_memref(gop2->id), current);
          //TODO: Announce delta
        }

      if(gop->location_key.type == String && !memref_equal(gop2->location_key,locp->key))
        {
          memref current = hash_get(vm->u_locs, gop2->location_key);
          location *lc = deref(&current);
          hash_remove(lc->objects, int_to_memref(gop2->id));
          //TODO: announce
        }

      if(loc.type == 0 )
        {
          //TODO: handle this case. should we remove from universe here?
          DIE("not implemented moving game object to null");
        }
  
      // add object to the new location
      gop2->location_key = locp->key;
      hash_set(locp->objects, int_to_memref(gop2->id), current);

      //TODO: announce          
      
    }
  
}

int step(vm* const vm, int fiber_index)
{
  exec_context* ec;
  REFRESH_EC;
  //printf("%#08x", ec->pc);
  enum opcode o = *(char*)ra_nth(vm->program,ec->pc++);
  //  printf("  %i  ", o);
  int i;
  memref ma, mb, mc, md;
  memref *mp;
  memref str;
  gameobject *gop;
  location *locp, *locp2;
  locationref *locrefp;
  fiber *fib;
  scope *scope;

  //print_stack(ec);
    switch(o)
    {
    case brk:
      //software breakpoint: todo
      VL("\t\t!!brk Not implemented\n");
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
      TL("swapn\n");
      READ_INT;
      int x = i & 0xFFFF;
      int y = (i >> 16) & 0xFFFF;
      memref* rhead = stack_peek_ref(ec->eval_stack);
      memref temp = *(rhead - x);
      *(rhead - x) = *(rhead-y);
      *(rhead - y) = temp;            
      break;
      
    case dup:
      VL("dup \n");
      ma = PEEK;
      PUSH(ma);
      break;
      
    case ldval:
      READ_INT;
      VL("ldval %i\n",i);
      PUSH(int_to_memref(i));
      break;
      
    case ldvals:
      VL("ldvals ");
      READ_STR;
      #ifdef VM_DEBUG
      ra_wl(str);
      #endif
      PUSH(str);
      break;
      
    case ldvalb:
      VL("ldvalb\n");
      READ_INT;
      PUSH(int_to_memref(i));        
      break;
      
    case ldvar:
      VL("ldvar ");
      READ_STR;
      VL("read str\n");
      #ifdef VM_DEBUG
      ra_wl(str);
      #endif
      ma = load_var(current_scope(ec),str);
      #ifdef VM_DEBUG
      printf("loaded var ");
      ra_w(str);
      VL(" with a value of %i\n", ma.data.i);
      #endif

      PUSH(ma);
      //   printf("stack now\n");
      REFRESH_EC;
      //      print_stack(ec);
    
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
      VL("ldprop\n");
      //      print_stack(ec);
      ma = POP;
      mb = POP;
      PUSH(load_prop(ma,mb));     
      break;
      
    case p_ldprop:
      VL("p_ldprop\n");
      ma = POP; //key
      mb = PEEK;//obj
      PUSH(load_prop(ma,mb));
      break;
      
    case stprop:
      VL("stprop\n");
      ma = POP; //val
      mb = POP; //key
      mc = POP; //obj
      store_prop(vm,ma,mb,mc);
      break;
      
    case p_stprop:
      VL("p_stprop\n");
      ma = POP; //val
      mb = POP; //key
      mc = PEEK; //obj
      store_prop(vm,ma,mb,mc);
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
      assert(mp->type == Int32);
      mp->data.i = -mp->data.i;
      break;
       
    case add:
      VL("add\n");
      ma = POP;
      mb = POP;      
      if(ma.type == Int32 && mb.type == Int32)
        {
          PUSH(int_to_memref( ma.data.i + mb.data.i));
        }
      // you can "add" strings
      else if(ma.type == String && mb.type == String)
        {
          int lena = ra_count(ma);
          int lenb = ra_count(mb);
          mc = ra_init_raw(sizeof(char),lena+lenb, String);
          ra_consume_capacity(mc);
          char* new_data = ra_data(mc);
          char* a_data = ra_data(ma);
          char* b_data = ra_data(mb);
          memcpy(new_data,b_data,lenb);
          memcpy(new_data+lenb,a_data,lena);
          REFRESH_EC;
          PUSH(mc);
        }
      // you can also "add" an int to a string (1 + "a" == "1a")
      else if(ma.type == Int32 && mb.type == String)
        {
          //printf("BREAK HERE!\n");
          i = snprintf(NULL, 0,"%d",ma.data.i);
          md = ra_init_raw(sizeof(char),i,String);
          ra_consume_capacity(md);
          char* a_data = ra_data(md);
          sprintf(a_data, "%d", ma.data.i);
          int lenb = ra_count(mb);
          mc = ra_init_raw(sizeof(char),i+lenb, String);
          ra_consume_capacity(mc);
          char* new_data = ra_data(mc);
          char* b_data = ra_data(mb);
          memcpy(new_data,b_data,lenb);
          memcpy(new_data+lenb,a_data,i);
          REFRESH_EC;
          PUSH(mc);
        }
      // TODO: reverse case of above, String + Int
      else
        {
          DIE("add only supports ints and strings not %i + %i!",ma.type, mb.type);
        }
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
      DIE("\t\t!!pow Not implemented\n");
      break;
    case tostr:
      DIE("\t\t!!tostr Not implemented\n");
      break;
    case toint:
      DIE("\t\t!!toint Not implemented\n");
      break;
    case rndi:
      TL("\t\trndi");
      ma = POP;
      assert(ma.type == Int32);
      mb = POP;
      assert(mb.type == Int32);
      int mini = ma.data.i;
      int maxi = mb.data.i;
      int diff = maxi - mini;
      if(diff == 0)
        {
          DIE("divide by zero in rndi");
        }
      int rnd = rand() % diff;
      PUSH(int_to_memref(rnd + mini));
      break;
    case startswith:
      DIE("\t\t!!startswith Not implemented\n");
      break;
    case p_startswith:
      DIE("\t\t!!p_startswith Not implemented\n");
      break;
    case endswith:
      DIE("\t\t!!endswith Not implemented\n");
      break;
    case p_endswith:
      DIE("\t\t!!p_endswith Not implemented\n");
      break;
    case contains:
      VL("contains\n");
      ma = POP;  //val
      mb = POP;  //obj
      if(ref_contains(mb,ma))
        {
          PUSH(int_to_memref(1));
        }
      else
        {
          PUSH(int_to_memref(0));
        }
      break;
      
    case p_contains:
      VL("p_contains\n");
      ma = POP;  //val
      mb = PEEK;  //obj
      if(ref_contains(mb,ma))
        {
          PUSH(int_to_memref(1));
        }
      else
        {
          PUSH(int_to_memref(0));
        }
      break;
      
    case indexof:
      DIE("\t\t!!indexof Not implemented\n");
      break;
    case p_indexof:
      DIE("\t\t!!p_indexof Not implemented\n");
      break;
    case substring:
      DIE("\t\t!!substring Not implemented\n");
      break;
    case p_substring:
      DIE("\t\t!!p_substring Not implemented\n");
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
      break;
      
    case cgt:
      VL("cgt\n");
      ma = POP;
      mb = POP;
      PUSH(int_to_memref(memref_gt(ma,mb)));
      break;
      
    case cgte:
      VL("cgte\n");
      ma = POP;
      mb = POP;
      PUSH(int_to_memref(memref_gte(ma,mb)));
      break;

    case clt:
      VL("clt\n");
      ma = POP;
      mb = POP;
      PUSH(int_to_memref(memref_lt(ma,mb)));
      break;
      
    case clte:
      VL("clte\n");
      ma = POP;
      mb = POP;
      PUSH(int_to_memref(memref_lte(mb,ma)));
      break;
      
    case beq:
      VL("beq\n");
      READ_INT;
      if(memref_equal(POP,POP))
        {
          ec->pc += i - 5;
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
      VL("bgt\n");
      READ_INT;
      ma = POP;
      mb = POP;
      if(memref_gt(ma,mb))
        {
          VL("branch taken\n");
          ec->pc += i - 5;
        }
      break;
      
    case blt:
      VL("blt\n");
      READ_INT;
      ma = POP;
      mb = POP;
      if(memref_lt(ma,mb))
        {
          VL("branch taken\n");
          ec->pc += i - 5;
        }
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
      VL("isobj\n");
      ma = POP;
      PUSH(int_to_memref(ma.type == GameObject));              
      break;
      
    case isint:
      VL("isint\n");
      ma = POP;
      PUSH(int_to_memref(ma.type == Int32));        
      break;
      
    case isbool:
      DIE("\t\t!!isbool Not implemented\n");
      break;
      
    case isloc:
      DIE("\t\t!!isloc Not implemented\n");
      break;
      
    case isarray:
      VL("isarray\n");
      ma = POP;
      PUSH(int_to_memref(ma.type == Array));        
      break;
      
    case createobj:
      VL("createobj\n");
      ma = malloc_go(vm->u_max_id++);
      REFRESH_EC;
      PUSH(ma);
      break;
      
    case cloneobj:
      VL("cloneobj\n");
      //shallow copy for now, not sure what this should be
      ma = POP;
      assert(ma.type == GameObject);
      mb = malloc_go(vm->u_max_id++);
      go* g = deref(&ma);
      go* gn = deref(&mb);
      memref kvps = hash_get_key_values(g->props);
      int max = ra_count(kvps);
      for(int i = 0; i < max; i++)
        {
          memref kvp_ref = ra_nth_memref(kvps,i);
          assert(kvp_ref.type == KVP);
          key_value* kvp = deref(&kvp_ref);
          hash_set(gn->props,kvp->key,kvp->val);
        }
      REFRESH_EC;
      PUSH(mb);
      break;
    case getobj:
      VL("getobj\n");
      ma = POP;
      mb = hash_get(vm->u_objs,ma);
      PUSH(mb);
      break;
    case getobjs:
      DIE("\t\t!!getobjs Not implemented\n");
      break;
    case delprop:
      DIE("\t\t!!delprop Not implemented\n");
      break;
    case p_delprop:
      DIE("\t\t!!p_delprop Not implemented\n");
      break;
    case delobj:
      DIE("\t\t!!delobj Not implemented\n");
      break;
    case moveobj:
      TL("\t\t!!moveobj\n");
      ma = POP; // loc
      mb = POP; // obj
      mb = unpack_location(vm,ma);
      move_gameobject(vm,mb,ma);
      break;
    case p_moveobj:
      TL("\t\t!!p_moveobj\n");
      ma = POP; // loc
      mb = PEEK; // obj
      mb = unpack_location(vm,ma);
      move_gameobject(vm,mb,ma);
      break;
    case createarr:
      VL("createarr\n");
      ma = ra_init(sizeof(memref),3);
      REFRESH_EC;
      PUSH(ma);
      break;
    case appendarr:
      VL("appendarr\n");
      ma = POP;
      mb = POP;
      #if DEBUG
      if(mb.type != Array)
        {
          VL("Execpted array in p_appendlist but got %i\n",mb.type);
        }
      #endif
      ra_append_memref(mb,ma);

      break;
    case p_appendarr:
      VL("p_appendarr\n");
      ma = POP;
      mb = PEEK;      
      #if DEBUG
      if(mb.type != Array)
        {
          VL("Execpted arr in p_appendlist but got %i\n",mb.type);
        }
      #endif
      ra_append_memref(mb,ma);
      break;
    case prependarr:
      DIE("\t\t!!prependarr Not implemented\n");
      break;
    case p_prependarr:
      DIE("\t\t!!p_prependarr Not implemented\n");
      break;
    case removearr:
      VL("removearr\n");
      ma = POP;
      mb = POP;
      assert(mb.type == Array);

      max = ra_count(mb);
      for(i = 0; i < max; i++)
        {
          if(memref_equal(ma, ra_nth_memref(mb,i)))
            {
              ra_remove(mb, i, 1);
              max--;
              i--;
            }
        }

      break;
    case p_removearr:
      DIE("\t\t!!p_removearr Not implemented\n");
      break;

    case len:
      VL("len\n");
      ma = POP;
      assert(ma.type == Array || ma.type == String);
      PUSH(int_to_memref(ra_count(ma)));
      break;

    case p_len:
      VL("p_len\n");
      ma = PEEK;
      assert(ma.type == Array || ma.type == String);
      PUSH(int_to_memref(ra_count(ma)));
      break;

    case index:
      VL("index\n");
      ma = POP;
      mb = POP;
      if(mb.type == Array)
        {
          PUSH(ra_nth_memref(mb,ma.data.i));
        }
      else if(mb.type == String)
        {
          i = *(char*)ra_nth(mb,ma.data.i);
          PUSH(int_to_memref(i));
        }
      else
        {
          assert(0);
        }
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
      assert(mb.type == Int32);
      assert(mc.type == Array);
      #endif
      ra_set_memref(mc,mb.data.i,ma);
      break;
      
    case keys:
      VL("keys\n");
      ma = POP;
      assert(ma.type == Hash || ma.type == GameObject || ma.type == Location || ma.type == LocationReference);
      if(ma.type == Hash)
        {
          mb = hash_get_keys(ma);
        }
      else if(ma.type == GameObject)
        {
          gop = deref(&ma);
          mb = hash_get_keys(gop->props);
        }
      else if(ma.type == Location)
        {
          locp = deref(&ma);
          mb = hash_get_keys(locp->props);
        }
      else if(ma.type == LocationReference)
        {
          locrefp = deref(&ma);
          mb = hash_get_keys(locrefp->props);
        }
      else
        {
          assert(0);
        }
          
      REFRESH_EC;
      PUSH(mb);
      break;
    case values:
      VL("values \n");
      ma = POP;
      assert(ma.type == Hash || ma.type == GameObject);
      if(ma.type == Hash)
        {
          mb = hash_get_values(ma);
        }
      else if(ma.type == GameObject)
        {
          gop = deref(&ma);
          mb = hash_get_keys(gop->props);
        }
      else
        {
          assert(0);
        }
      REFRESH_EC;
      PUSH(mb);
      break;
    case syncprop:
      DIE("\t\t!!syncprop Not implemented\n");
      break;
    case getloc:
      TL("\t\tgetloc\n");
      ma = POP;
      assert(ma.type == String || ma.type == LocationReference || ma.type == GameObject);      
      if(ma.type == String)
        {
          mb = ma;
        }
      else if(ma.type == LocationReference)
        {
          locrefp = deref(&ma);
          mb = locrefp->target_key;
        }
      else if(ma.type == GameObject)
        {
          gop = deref(&ma);
          if(gop->location_key.type == 0)
            {
              DIE("gameobject did not exist in a location");
            }
          else
            {
              mb = gop->location_key;
            }
        }
      PUSH(hash_get(vm->u_locs, mb));
      break;
    case genloc:
      TL("\t\tgenloc\n");
      ma = POP;
      assert(ma.type == String);
      assert(!hash_contains(vm->u_locs,ma));
      mb = malloc_loc(ma);
      hash_set(vm->u_locs, ma, mb);
      PUSH(mb);
      //TODO: annnouce delta
      break;
    case genlocref:
      TL("\t\tgenlocref\n");
      //ma = POP;
      /* assert(ma.type == String); */
      /* assert(!hash_contains(vm->u_locrefs,ma)); */
      mb = malloc_locref();
      locrefp = deref(&mb);
      locrefp->id = vm->u_max_id++;
      //hash_set(vm->u_locrefs, mb, ma);
      PUSH(mb);
      //TODO: announce delta            
      break;
    case setlocsibling:
      TL("\t\tsetlocsibling\n");
      // key :: locref :: relation loc :: host loc      
      ma = POP;
      assert(ma.type == String);
      mb = POP;
      assert(mb.type == LocationReference);      
      locrefp = deref(&mb);
      mc = POP;
      assert(mc.type == Location);
      mc = unpack_location(vm,mc);
      locp = deref(&mc);
      md = POP;
      assert(md.type == Location);
      md = unpack_location(vm,md);
      locp2 = deref(&md);

      // assign the locref to the host using the key
      // point the locref to the relation.

      if(hash_contains(locp2->siblings, ma))
        {
          DL("warning: location already exists in setlocsibling");
        }
      hash_set(locp2->siblings, ma, mb);

      locrefp->target_key = locp->key;
      
      //TODO: announce delta            
      break;
    case p_setlocsibling:
      TL("\t\tp_setlocsibling\n");
      // key :: locref :: relation loc :: host loc      
      ma = POP;
      assert(ma.type == String);
      mb = POP;
      assert(mb.type == LocationReference);      
      locrefp = deref(&mb);
      mc = POP;
      assert(mc.type == Location);
      mc = unpack_location(vm,mc);
      locp = deref(&mc);
      md = PEEK;
      assert(md.type == Location);
      md = unpack_location(vm,md);
      locp2 = deref(&md);

      // assign the locref to the host using the key
      // point the locref to the relation.
      locrefp->target_key = locp->key;

      if(hash_contains(locp2->siblings, ma))
        {
          DL("warning: location already exists in p_setlocsibling");
        }
      hash_set(locp2->siblings, ma, mb);      
      //TODO: announce delta            
      break;
    case setlocchild:  
      TL("\t\tsetlocchild\n");
       // key :: locref :: child loc :: parent loc      
      ma = POP;
      assert(ma.type == String);
      mb = POP;
      assert(mb.type == LocationReference);      
      locrefp = deref(&mb);
      mc = POP;
      mc = unpack_location(vm,mc);
      assert(mc.type == Location);
      locp = deref(&mc);
      md = POP;
      md = unpack_location(vm,md);
      assert(md.type == Location);
      locp2 = deref(&md);

      //link parent to child
      locrefp->target_key = locp->key;
      if(hash_contains(locp2->children, ma))
        {
          DL("warning: location already exists in setlochild");
        }
      hash_set(locp2->children, ma, mb);

      //link child to parent
      md = malloc_locref();
      locrefp = deref(&md);
      locrefp->id = vm->u_max_id++;
      locrefp->target_key = locp2->key;
      locp->parent = md;
            
      //TODO: announce delta
      break;
    case p_setlocchild:
      TL("\t\t!!p_setlocchild\n");
      // key :: childlocref :: child loc :: parent loc      
      ma = POP;
      assert(ma.type == String);
      mb = POP;
      assert(mb.type == LocationReference);      
      locrefp = deref(&mb);
      mc = POP;
      assert(mc.type == Location);
      mc = unpack_location(vm,mc);
      locp = deref(&mc);
      md = PEEK;
      assert(md.type == Location);
      md = unpack_location(vm,md);
      locp2 = deref(&md);

      //link parent to child
      locrefp->target_key = locp->key;      
      if(hash_contains(locp2->children, ma))
        {
          DL("warning: location already exists in p_setlocsibling");
        }
      hash_set(locp2->children, ma, mb);

      //link child to parent
      md = malloc_locref();
      locrefp = deref(&md);
      locrefp->id = vm->u_max_id++;
      locrefp->target_key = locp2->key;
      locp->parent = md;

      //TODO: Announce deltas
      break;
    case setlocparent:
      DIE("\t\t!!setlocparent Not implemented\n");
      break;
    case p_setlocparent:
      DIE("\t\tp_setlocparent  implemented\n");
      break;
    case getlocsiblings:
      TL("\t\tgetlocsiblings\n");
      ma = POP;
      ma = unpack_location(vm,ma);
      locp = deref(&ma);
      PUSH(locp->siblings);
      break;
    case p_getlocsiblings:
      TL("\t\tp_getlocsiblings\n");
      ma = PEEK;
      ma = unpack_location(vm,ma);
      locp = deref(&ma);
      PUSH(locp->siblings);
      break;
    case getlocchildren:
      DL("\t\t!!getlocchildren \n");
      ma = POP;
      ma = unpack_location(vm,ma);
      locp = deref(&ma);
      PUSH(locp->children);      
      break;
    case p_getlocchildren:
      TL("\t\t!!p_getlocchildren\n");
      ma = PEEK;
      ma = unpack_location(vm,ma);
      locp = deref(&ma);
      PUSH(locp->children);      
      break;
    case getlocparent:
      DIE("\t\t!!getlocparent Not implemented\n");
      break;
    case p_getlocparent:
      DIE("\t\t!!p_getlocparent Not implemented\n");
      break;
    case setvis:
      DIE("\t\t!!setvis Not implemented\n");
      break;
    case p_setvis:
      DIE("\t\t!!p_setvis Not implemented\n");
      break;
    case adduni:
      DIE("\t\t!!adduni Not implemented\n");
      break;
    case deluni:
      DIE("\t\t!!deluni Not implemented\n");
      break;
    case splitat:
      VL("splitat\n");
      ma = POP;  //bottom bool
      mb = POP;  //nth
      mc = POP;  //list
      assert(mc.type == Array);
      //don't allow mutating strings right now
      //since they might be keys 
      if(ma.data.i > 0)
        {
          //remove from start              
          md = ra_split_bottom(mc,mb.data.i);
          REFRESH_EC;
          PUSH(md);
        }
      else
        {                 
          //remove from end
          md = ra_split_top(mc,mb.data.i);
          REFRESH_EC;
          PUSH(md);
        }      
      break;
      
    case shuffle:
      DIE("\t\t!!shuffle Not implemented\n");
      break;
    case sort:
      DIE("\t\t!!sort Not implemented\n");
      break;
    case sortby:
      DIE("\t\t!!sortby Not implemented\n");
      break;
    case genreq:
      // a request is defined as an array of strings where
      // the first string is the title, and sucessive pairs
      // of strings represents the keys and descriptions
      VL("genreq\n");
      ma = POP;
      assert(ma.type == String);
      mb = ra_init(sizeof(memref),5);
      ra_append_memref(mb,ma);
      PUSH(mb);
      break;
    case addaction:
      VL("addaction\n");
      ma = POP;
      assert(ma.type == String); //can remove this restriciton later
      mb = POP;
      assert(mb.type == String); //can remove this restriciton later
      mc = POP;
      assert(mc.type == Array);
      ra_append_memref(mc,ma);
      ra_append_memref(mc,mb);
      break;
    case p_addaction:
      VL("addaction\n");
      ma = POP;
      assert(ma.type == String); //can remove this restriciton later
      mb = POP;
      assert(mb.type == String); //can remove this restriciton later
      mc = PEEK;
      assert(mc.type == Array);
      ra_append_memref(mc,ma);
      ra_append_memref(mc,mb);
      break;

    case suspend:
      VL("suspend\n");
      exec_context ec_new = clone_current_exec_context(vm, fiber_index);
      patch_stack_pool_pointers(vm);
      fib = get_fiber(vm,fiber_index);
      ra_append(fib->exec_contexts, &ec_new);
      fib = get_fiber(vm,fiber_index);
      REFRESH_EC;      
      ma = POP;  // clientid
      assert(ma.type == String);
      mb = POP;  // request
      assert(mb.type == Array);
      int len = ra_count(mb);
      assert(len % 2 != 0);
      mc = ra_init_raw(sizeof(char),1024,String);
      char* text = "{ \"t\":\"request\",\"header\":\"";
      ra_append_str(mc,text, strlen(text));
      ra_append_ra_str(mc, ra_nth_memref(mb,0));
      text = "\", \"choices\":[";
      ra_append_str(mc,text, strlen(text));

      fib = get_fiber(vm,fiber_index);
      fib->valid_responses = hash_init(3);

      if(ra_count(fib->exec_contexts) > 2)
        {
          text = "{\"id\":\"__UNDO__\",\"text\":\"UNDO\"},";
          ra_append_str(mc,text, strlen(text));
          hash_set(fib->valid_responses,ra_init_str("__UNDO__"),ra_init_str("UNDO"));
        }
      for(int i = 1; i < len; i+=2)
        {
          hash_set(fib->valid_responses,ra_nth_memref(mb,i),ra_nth_memref(mb,i+1));
          text = "{\"id\":\"";
          ra_append_str(mc,text, strlen(text));
          ra_append_ra_str(mc, ra_nth_memref(mb,i));
          text = "\",\"text\":\"";
          ra_append_str(mc,text, strlen(text));
          ra_append_ra_str(mc, ra_nth_memref(mb,i+1));
          if(i+2 < len)
            {
              text = "\"},";
              ra_append_str(mc,text, strlen(text));
            }
          else
            {
              text = "\"}";
              ra_append_str(mc,text, strlen(text));
            }
          //careful - re-get the fiber incase the dynamic pool moved
          fib = get_fiber(vm,fiber_index);
        }

      text = "]}";
      ra_append_str(mc,text, strlen(text));

      printf("\n");
      text = "]}";
      ra_append_str(mc,text, strlen(text));

      printf("sending suspend message to client ");
      ra_wl(mc);
      printf("\n");
      
      fib = get_fiber(vm,fiber_index);
      fib->awaiting_response = Choice;
      fib->waiting_client = ma;
      fib->waiting_data = mc;

      //todo: we need to include the fiber id in the message
      zmq_msg_t msg_id = ra_to_msg(ma);
      zmq_msg_t msg_type;
      zmq_msg_init_size(&msg_type,1);
      char* data = zmq_msg_data(&msg_type); 
      *data = (char)Data;
      zmq_msg_t msg_data = ra_to_msg(mc);

      zmq_msg_send(&msg_id, vm->ip_machine, ZMQ_SNDMORE);
      zmq_msg_send(&msg_type, vm->ip_machine, ZMQ_SNDMORE);
      zmq_msg_send(&msg_data, vm->ip_machine, 0);      
      return 1;
      
    case cut:
      VL("cut\n");
      fib = get_fiber(vm,fiber_index);
      int n = ra_count(fib->exec_contexts) - 1;
      exec_context* current = ra_nth(fib->exec_contexts,n);
      ra_set(fib->exec_contexts,0,current);
      refarray* ra = deref(&fib->exec_contexts);
      ra->element_count = 1;
      break;
    case say:
      DL("\t\t!!say Not implemented\n");
      break;

    case pushscope:
      VL("pushscope\n");
      ma = current_scope_ref(ec);
      mc = init_scope();
      scope = deref(&mc);      
      scope->closure_scope = ma;
      scope->return_address = 0;
      ra_append_memref(ec->scopes,mc);
      break;
      
    case popscope:
      VL("popscope\n");
      ra_dec_count(ec->scopes);
      break;
      
    case lambda:
      VL("lambda\n");
      READ_INT;
      memref f = init_function(ec,i);
      PUSH(f);
      break;
      
    case apply:
      VL("apply\n");
      ma = POP; // arg
      mb = POP; // func
      mc = init_scope();
      scope = deref(&mc);
      //assume func for now (support objs etc later)
      function* apply_f = deref(&mb);
      scope->closure_scope = apply_f->closure_scope;
      VL("scope return address is %i\n", ec->pc);
      scope->return_address = ec->pc;
      ra_append_memref(ec->scopes,mc);
      REFRESH_EC;
      PUSH(ma);
      ec->pc = apply_f->address;
      break;
      
    case ret:
      VL("ret\n");
      //      VL("scope count is %i\n", ra_count(ec->scopes));
      if(ra_count(ec->scopes) == 1)
        {
          printf("Program end.\n");
          ma = current_scope_ref(ec);
          scope = deref(&ma);
          ra_dec_count(ec->scopes);
          DL("scope count is %i\n", ra_count(ec->scopes));
          return 2;      
        }
      else
        {
          //walk backwards down the scopes until we find one
          //with a return address or fiber entry point
          while(1)
            {
              ma = current_scope_ref(ec);
              scope = deref(&ma);
              ra_dec_count(ec->scopes);
              if(scope->return_address != 0)
                {
                  if(scope->is_fiber)
                    {
                      printf("returning from fiber %i\n", fiber_index);
                      ra_remove(vm->fibers, fiber_index, 1);
                      if(ra_count(vm->fibers) == 1)
                        {
                          printf("resuimng core execution\n");
                          fib = get_fiber(vm, 0);
                          fib->awaiting_response = None;
                        }
                      return 3;
                    }
                  else
                    {
                      //if this is a fiber entry point, we are done
                      //and can end this fibe.
                      VL("returning to address is %i\n", scope->return_address);
                      ec->pc = scope->return_address;
                    }
                  break;
                }
            }
          break;
        }
      
    case dbg:      
      VL("dbg\n");
      ma = POP;
#if DEBUG
      print_ref(ma);
#endif
      break;
      
    case dbgl:
      VL("dbgl\n");
      ma = stack_pop(ec->eval_stack);
#if DEBUG
      print_ref(ma);
      putchar('\n');
#endif
      break;

    case getraw:
      DL("getraw\n");
      ma = POP;
      ra_wl(ma);
      fib = get_fiber(vm,fiber_index);
      fib->awaiting_response = RawData;
      fib->waiting_client = ma;
      fib->waiting_data.type = 0;
      fib->valid_responses = hash_init(3); //really need to implement clear()

      //todo: we need to include the fiber id in the message
      zmq_msg_t msg_id_raw = ra_to_msg(ma);
      zmq_msg_t msg_type_raw;
      zmq_msg_init_size(&msg_type_raw,1);
      char* data_raw = zmq_msg_data(&msg_type_raw); 
      *data_raw = (char)Raw;

      zmq_msg_send(&msg_id_raw, vm->ip_machine, ZMQ_SNDMORE);
      zmq_msg_send(&msg_type_raw, vm->ip_machine, 0);
      return 1;

    case fork:
      //fork accepts a function and and an argument.
      //assert there is only a single execution context and this is root fiber.
      //clone the execution context into a new fiber and apply
      //the argument to the function in it, marking the new scope as a fiber
      //entry point.
      assert(fiber_index == 0);
      fib = get_fiber(vm,fiber_index);
      int max_ec = ra_count(fib->exec_contexts);
      assert(max_ec == 1);

      ma = POP; //arg
      mb = POP; //func

      assert(mb.type == Function);

      
      fiber new_f = init_fiber(max_ec);
      patch_stack_pool_pointers(vm);
      new_f.valid_responses.type = 0;
      new_f.waiting_client.type = 0;
      new_f.waiting_data.type = 0;

      exec_context* new_ec = current_ec(&new_f);

      mc = init_scope();
      scope = deref(&mc);
      scope->is_fiber = true;
      apply_f = deref(&mb);
      scope->closure_scope = apply_f->closure_scope;
      new_ec->pc = apply_f->address;
      scope->return_address = new_ec->pc;
      ra_append_memref(new_ec->scopes,mc);      
      new_ec = current_ec(&new_f);
      stack_push(new_ec->eval_stack,ma);
      ra_append(vm->fibers,&new_f);
      break;


    case join:
      DL("JOIN\n");
      //assert this is the root fiber.
      //put this fiber into the waiting state.
      assert(fiber_index == 0);
      fib = get_fiber(vm,fiber_index);
      fib->awaiting_response = Join;
      return 1;

    case cons:
      VL("cons \n");
      ma = POP;
      mb = POP;
      //      assert(mb.type == List);
      mc = rl_init_h_t(ma, mb);
      PUSH(mc);
      break;

    case tail:
      VL("tail \n");
      ma = POP;
      //      DL("til type is %i\n", ma.type);
      assert(ma.type == List);
      mb = rl_tail(ma);
      /* mc = rl_tail(mb); */
      /* list* lst = deref(&mc); */
      /* if(lst->head.type == 0 && mc->head.type == 0) */
      /*   { */
      /*     //this is the end of the list so push the value */
      /*     //free of its list container */
          
      /*   } */
      /* else */
      /*   { */
          PUSH(mb);
          //        }
      break;


    case head:
      VL("head \n");
      ma = POP;
      /* DL("head type was %i\n", ma.type); */
      assert(ma.type == List);
      mb = rl_head(ma);
      PUSH(mb);
      break;
      
    case islist:
      VL("islist \n");
      ma = POP;
      mb = int_to_memref(ma.type == List ? 1 : 0);
      PUSH(mb);
      break;

    case createlist:
      VL("createlist \n");
      ma = rl_init();
      PUSH(ma);
      break;
      
    case isfunc:
      VL("isfunc\n");
      ma = POP;
      PUSH(int_to_memref(ma.type == Function ? 1 : 0));
    }

 
  
  return 0;
}


void run(vm* vm)
{
  /* fiber* f = (fiber*)ra_nth(vm->fibers,0); */
  /* exec_context* ec = (exec_context*)ra_nth(f->exec_contexts,0); */
  /* DL("stack offset is %i\n", stack_head_offset(ec->eval_stack)); */
  /* while(step(vm,ec) == 0) */
  /*   { */
  /*     vm->cycle_count++; */
  /*     if(vm->cycle_count % 100 == 0) */
  /*       { */
  /*         gc_mark_n_sweep(vm); */

  /*       } */
  /*   } */

  /* gc_print_stats(); */
  /* VL("freeing scope..\n"); */

  /* /\* ra_dec_count(ec->scopes); *\/ */

  /* gc_mark_n_sweep(vm); */
  /* gc_print_stats(); */
 

  /* VL("run finihsed\n"); */
}

