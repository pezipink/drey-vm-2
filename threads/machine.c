#include <winsock2.h>
#include <windows.h>
#include "machine.h"
#include <time.h>
#include "..\zmq.h"
#include "..\global.h"
#include "..\zhelpers.h"
#include "..\vm\vm.h"
#include "..\memory\gc.h"
#include "..\datastructs\refhash.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

int parse_response(char** response, int* responseLen, char* data)
{
  //response should be json with an id field and string value.
  //test for this directly for now
  char* find = "\"id\"";
  char* start = strstr(data, find);

  if(!start)
    {
      return 0;
    }

  start += 4;
  int state = 0;  

  while(1)
    {
      while(isspace(*start))
        {
          start++;
          if(state == 2)
            {
              (*responseLen)++;
            }
        }
      if(*start == 0)
        {
          return 0;
        }
      switch(state)
        {
        case 0:
          if(*start == ':')
            {
              state = 1;
              start++;
            }
          else
            {
              return 0;
            }
          break;

        case 1:
          if(*start == '\"')
            {
              start++;
              *response =  start;
              state = 2;              
            }
          else
            {
              return 0;
            }
          break;
        case 2:
          if(*start == '\"')
            {
              return 1;
            }
          else
            {
              (*responseLen)++;
              start++;
            }
          break;
        }
    }
  
}

DWORD WINAPI machine_thread(LPVOID context)
{
  printf("machine thread created context at %p\n",context);
  void* ip_machine = zmq_socket(context, ZMQ_PAIR);
  zmq_connect(ip_machine,"inproc://machine");
  printf("machine signalling mediator\n");

  vm machine = init_vm(ip_machine);
  s_send(ip_machine, "READY");

  zmq_pollitem_t items [] = {
    { ip_machine, 0, ZMQ_POLLIN, 0 }
  };

  int count = 0;
  while(1)
    {
      zmq_poll (items, 1, 10);
      if (items [0].revents & ZMQ_POLLIN)
        {
          zmq_msg_t msg_id;
          zmq_msg_t msg_type;
          zmq_msg_t msg_data;
          int id_size = 0;
          int size = 0;
          zmq_msg_init(&msg_id);
          id_size = zmq_msg_recv(&msg_id,ip_machine,0);          
          
          bool more = false;
          bool hasData = false;
          bool hasType = false;
          size_t optLen = sizeof(more);
          enum MessageType type;
          zmq_getsockopt(ip_machine,ZMQ_RCVMORE,&more, &optLen);
          if(more) //this must be more
            {
              zmq_msg_init(&msg_type);
              size = zmq_msg_recv(&msg_type,ip_machine,0);
              hasType = true;
              char* data = zmq_msg_data(&msg_type);
              //              printf("mediator: second frame was %i bytes with value %i\n", size, *data);
              switch(*data)
                {
                case Connect:
                  vm_client_connect(&machine, zmq_msg_data(&msg_id), id_size);
                  break;

                case Data:
                case Debug:
                case Raw:
                  zmq_msg_init(&msg_data);
                  size = zmq_msg_recv(&msg_data,ip_machine,0);
                  //                printf("machine: third frame was %i bytes\n", size);

                  if(*data == Raw)
                    {
                      vm_handle_raw(&machine, zmq_msg_data(&msg_id), id_size, zmq_msg_data(&msg_data), zmq_msg_size(&msg_data));
                    }
                  else
                    {
                      //extract response.
                      char* response = 0;
                      int responseLen = 0;
                      if(parse_response(&response, &responseLen, zmq_msg_data(&msg_data)))
                        {
                          //      printf("parse response successfully with %i %i\n", *response, responseLen);
                          vm_handle_response(&machine, zmq_msg_data(&msg_id), id_size, response, responseLen);
                        }
                      else
                        {
                          printf("did not understand memssage from client\n");
                        }
                    }
                  hasData = true;

                  break;

                default:
                  printf("unsupported message type made its way into the machine thread!!\n");
                  break;
                }
            }
          else
            {
              assert(0);
            }
                    
          if(hasType)
            {
              zmq_msg_close(&msg_type);
            }

          if(hasData)
            {
              zmq_msg_close(&msg_data);
            }          
          
          zmq_msg_close(&msg_id);
          
        }
      else if(!machine.game_over && machine.num_players == machine.req_players)
        {
          //no messages to process, execute the VM up
          //to the next suspend call.

          int count = ra_count(machine.fibers);
          for(int i = 0; i < count; i++)
            {
              fiber* f  = (fiber*)ra_nth(machine.fibers,i);
              if(f->awaiting_response == None)
                {
                  printf("executing fiber %i\n", i);
                  while(1)
                    {
                      int ret = step(&machine, i);
                      machine.cycle_count++;
                      //                          gc_mark_n_sweep(&machine);
                      if(machine.cycle_count % 100 == 0)
                        {

                        }
                      if(ret == 2)
                        {
                          machine.game_over = true;
                          machine.u_objs = hash_init(3);
                          //                          machine.string_table = hash_init(3);
                          break;
                        }
                      else if(ret != 0)
                        {
                          break;
                        }
                    }
                  /* gc_print_stats(); */
                  /* gc_mark_n_sweep(&machine); */
                  /* gc_print_stats(); */
        
                }
            }

          //todo: compress dynamic memory
        }
    }
}


