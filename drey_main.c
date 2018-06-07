#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "memory\manager.h"
#include "datastructs\refhash.h"
#include "datastructs\refstack.h"
#include "datastructs\reflist.h"
#include "global.h"
#include "zmq.h"
#include "zhelpers.h"
#include "threads\mediator.h"


int main(int argc, char *argv[])
{

  printf("entry\n");

  TL("test%i\n", 0);
  int memory = 0;
  
  fixed_pool_init(&ref_memory,sizeof(ref),1); //10mb of refs
  fixed_pool_init(&hash_memory,sizeof(refhash),1);
  fixed_pool_init(&scope_memory,sizeof(scope),1);
  fixed_pool_init(&func_memory,sizeof(function),1);
  fixed_pool_init(&stack_memory,sizeof(refstack),10);
  fixed_pool_init(&kvp_memory,sizeof(key_value),1);
  dyn_pool_init(&dyn_memory,sizeof(int) * 1024 * 1024);
  fixed_pool_init(&go_memory,sizeof(gameobject),1);
  fixed_pool_init(&loc_memory,sizeof(location),10);
  fixed_pool_init(&loc_ref_memory,sizeof(locationref),10);
  fixed_pool_init(&list_memory,sizeof(list),1);

  
  
  // Prepare our context and sockets
  printf("start\n");
  void* context = zmq_ctx_new ();    

  printf("creating root inproc socket in main thread\n");
  void* ip_root = zmq_socket(context, ZMQ_PAIR);
  zmq_bind(ip_root, "inproc://root");
  
  HANDLE thread = CreateThread(NULL, 0, mediator_thread, context, 0, NULL);
  
  //wait for mediator signal
  char* str = s_recv(ip_root);
  printf("root received signal, continuing\n");
  free(str);
  void *tcp_router = zmq_socket (context, ZMQ_ROUTER);
  printf("binding tcp router socket\n");  
  zmq_bind (tcp_router, "tcp://*:5560");
  printf("socket bound\n");

  zmq_pollitem_t items [] = {
     { ip_root,   0, ZMQ_POLLIN, 0 },
     { tcp_router,   0, ZMQ_POLLIN, 0 },
  };

    
  while(1)
    {
      
      Sleep(10);
      //      printf("root: looking for message from mediator..\n");
      zmq_poll (items, 2, 10);
      if (items [0].revents & ZMQ_POLLIN)
        {
          //this is an internal message from the mediator thread.
          //forward it on to the router.
          bool more = false;
          size_t optLen = sizeof(more);
          while(1)
            {
              zmq_msg_t msg;
              zmq_msg_init(&msg);
              zmq_msg_recv(&msg, ip_root, 0);          
              zmq_msg_t cpy;
              zmq_msg_init(&cpy);
              zmq_msg_copy(&cpy, &msg);
              zmq_getsockopt(ip_root,ZMQ_RCVMORE,&more, &optLen);
              zmq_msg_close(&msg);              
              if(more)
                {
                  zmq_msg_send(&cpy, tcp_router, ZMQ_SNDMORE);
                }
              else
                {
                  zmq_msg_send(&cpy, tcp_router, 0);
                  break;
                }
            }         

        }
      
      if(items[1].revents & ZMQ_POLLIN)
        {
          // incoming message from a client
          //a valid message is formed of at least 2 frames
          //the first being the client / socket identifier
          //and the second being the message type.
          //some message types have a third frame.
          //and no messages have more.

          //here we validate the above and forward copies onto the
          //mediator thread.
          zmq_msg_t msg_identifier;
          zmq_msg_t msg_type;
          zmq_msg_t msg_data;
          int size = 0;
          zmq_msg_init(&msg_identifier);
          size = zmq_msg_recv(&msg_identifier,tcp_router,0);          
          //                   printf("root: external message of %i size\n", size);
          char* buffer = malloc(size+1);
          memset(buffer,0,size+1);
          memcpy(buffer,zmq_msg_data(&msg_identifier),size);
          //          printf("root: identifier was ");
          //          printf(buffer);
          //          printf("\n");
          free(buffer);
          bool invalid = false;
          bool more = false;
          bool hasData = false;
          bool hasType = false;
          size_t optLen = sizeof(more);
          zmq_getsockopt(tcp_router,ZMQ_RCVMORE,&more, &optLen);
          if(more)
            {
              zmq_msg_init(&msg_type);
              size = zmq_msg_recv(&msg_type,tcp_router,0);
              hasType = true;
              char* data = zmq_msg_data(&msg_type);
              //printf("root: second frame was %i bytes with value %i", size, *data);
              if(*data == Data || *data == Debug || *data == Raw)
                {
                  //TODO: limit the size of raw data to, say, 1mb
                  zmq_msg_init(&msg_data);
                  size = zmq_msg_recv(&msg_data,tcp_router,0);
                  printf("root: third frame was %i bytes", size);
                  hasData = true;
                }
              else if(*data < 0 || *data > 7)
                {
                  invalid = true;
                }
            }
          else
            {
              invalid = true;
            }


          //there should be no more data, otherwise its a funky message
          zmq_getsockopt(tcp_router,ZMQ_RCVMORE,&more, &optLen);
          if(more)
            {
              while(more)
                {
                  printf("root: throwing away another frame..\n");
                  zmq_msg_t msg_temp;
                  zmq_msg_init(&msg_temp);
                  size = zmq_msg_recv(&msg_temp,tcp_router,0);
                  zmq_msg_close(&msg_temp);
                  zmq_getsockopt(tcp_router,ZMQ_RCVMORE,&more, &optLen);
                }
            }
          else if(!invalid)
            {
              //message was good, forward them on
              zmq_msg_t msg_identifier_copy;
              zmq_msg_init(&msg_identifier_copy);
              zmq_msg_copy(&msg_identifier_copy, &msg_identifier);
              zmq_msg_send(&msg_identifier_copy, ip_root, ZMQ_SNDMORE);
              zmq_msg_t msg_type_copy;
              zmq_msg_init(&msg_type_copy);
              zmq_msg_copy(&msg_type_copy, &msg_type);
              zmq_msg_send(&msg_type_copy, ip_root, hasData ? ZMQ_SNDMORE : 0);
              if(hasData)
                {
                  zmq_msg_t msg_data_copy;
                  zmq_msg_init(&msg_data_copy);
                  zmq_msg_copy(&msg_data_copy, &msg_data);
                  zmq_msg_send(&msg_data_copy, ip_root, 0);
                }
              
            }
          
          if(hasType)
            {
              zmq_msg_close(&msg_type);
            }

          if(hasData)
            {
              zmq_msg_close(&msg_data);
            }          
          
          zmq_msg_close(&msg_identifier);
        }

          
    }
  printf("exit");
  return 0;
}
