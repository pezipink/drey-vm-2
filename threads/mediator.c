#include <winsock2.h>
#include <windows.h>
#include "mediator.h"
#include "machine.h"
#include <time.h>
#include "..\zmq.h"
#include "..\global.h"
#include "..\zhelpers.h"
#include <assert.h>
//artifical restriction
#define MAX_CLIENTS 32

typedef struct clientdata
{
  int id_len;
  char* id;
  time_t last_heartbeat;
  bool connected;
} clientdata;

typedef struct mediator
{
  clientdata clients[MAX_CLIENTS];
  int client_count;
} mediator;


clientdata* find_client(mediator* this, char* clientid, int len)
{
  for(int i = 0; i < this->client_count; i++)
    {
      clientdata* c = &this->clients[i];
      if(c->id_len == len && memcmp(c->id, clientid, len) == 0)
        {
          return c;
        }
    }
 
 return 0;
}

void client_heartbeat(mediator* this, zmq_msg_t* msg_id, zmq_msg_t* msg_hb, void* socket)
{
  clientdata* c = find_client(this, zmq_msg_data(msg_id), zmq_msg_size(msg_id));
  if(c)
    {
      c->last_heartbeat = time(0);
      // return heartbeat to the client
      zmq_msg_t msg_id_copy;
      zmq_msg_t msg_hb_copy;
      zmq_msg_init(&msg_id_copy);
      zmq_msg_init(&msg_hb_copy);
      zmq_msg_copy(&msg_id_copy,msg_id);
      zmq_msg_copy(&msg_hb_copy,msg_hb);
      zmq_msg_send(&msg_id_copy, socket, ZMQ_SNDMORE);
      zmq_msg_send(&msg_hb_copy, socket, 0);
    }
  else
    {
      printf("recieved heartbeat from unknown client\n");
    }    
}

void client_connect(mediator* this, zmq_msg_t* msg_id, zmq_msg_t* msg_type, void* machine_socket)
{
  if(this->client_count == MAX_CLIENTS)
    {
      printf("client connection refused, maximum clients connected!!\n");
      return;
    }

  char* clientid = zmq_msg_data(msg_id);
  int len = zmq_msg_size(msg_id);
  clientdata* c = find_client(this, clientid, len);
  
  if(c)
    {
      c->connected = true;
      char* buffer = malloc(len+1);
      memset(buffer,0,len+1);
      memcpy(buffer,clientid,len);
      printf("client \n");
      printf(buffer);
      printf("reconnected \n");
      c->last_heartbeat = time(0);
      free(buffer);
      //      return;
    }
  else
    {
      //connect this client
      clientdata* new_client = &this->clients[this->client_count++];
      new_client->id_len = len;  
      new_client->id = malloc(len);
      memcpy(new_client->id, clientid, len);
      new_client->last_heartbeat = time(0);
      new_client->connected = true;
    }
  
  //forward (re)connect to machine
  zmq_msg_t msg_id_copy;
  zmq_msg_t msg_type_copy;
  zmq_msg_init(&msg_id_copy);
  zmq_msg_init(&msg_type_copy);
  zmq_msg_copy(&msg_id_copy,msg_id);
  zmq_msg_copy(&msg_type_copy,msg_type);
  zmq_msg_send(&msg_id_copy, machine_socket, ZMQ_SNDMORE);
  zmq_msg_send(&msg_type_copy, machine_socket, 0);      
  
}

void check_heartbeats(mediator* this)
{
  int now = time(0);
  for(int i = 0; i < this->client_count; i++)
    {
      if(this->clients[i].connected && now - this->clients[i].last_heartbeat > 1)
        {
          this->clients[i].connected = false;
          printf("client ");
          char* buffer = malloc(this->clients[i].id_len+1);
          memset(buffer,0,this->clients[i].id_len+1);
          memcpy(buffer,this->clients[i].id,this->clients[i].id_len);
          printf(buffer);
          free(buffer);
          printf("disconnected\n");
        }
    }
}

DWORD WINAPI mediator_thread(LPVOID context)
{
  void* ip_machine = zmq_socket(context, ZMQ_PAIR);
  zmq_bind(ip_machine, "inproc://machine");
  HANDLE thread = CreateThread(NULL, 0, machine_thread, context, 0, NULL);
  char* str = s_recv(ip_machine);
  free(str);

  void* ip_root = zmq_socket(context, ZMQ_PAIR);
  zmq_connect(ip_root,"inproc://root");
  s_send(ip_root, "READY");

  mediator mediator_context;
  mediator_context.client_count = 0;
  
  zmq_pollitem_t items [] = {
    { ip_root,   0, ZMQ_POLLIN, 0 },
    { ip_machine, 0, ZMQ_POLLIN, 0 }
  };

  int count = 0;
  while(1)
    {
      count++;
      if(count % 100 == 0)
        {
          check_heartbeats(&mediator_context);
        }
      zmq_poll (items, 2, 10);
      if (items [0].revents & ZMQ_POLLIN)
        {
          zmq_msg_t msg_id;
          zmq_msg_t msg_type;
          zmq_msg_t msg_data;
          int id_size = 0;
          int size = 0;
          zmq_msg_init(&msg_id);
          id_size = zmq_msg_recv(&msg_id,ip_root,0);          
          
          bool more = false;
          bool hasData = false;
          bool hasType = false;
          size_t optLen = sizeof(more);
          enum MessageType type;
          zmq_getsockopt(ip_root,ZMQ_RCVMORE,&more, &optLen);
          if(more) //this must be more
            {
              zmq_msg_init(&msg_type);
              size = zmq_msg_recv(&msg_type,ip_root,0);
              hasType = true;
              char* data = zmq_msg_data(&msg_type);
              //              printf("mediator: second frame was %i bytes with value %i\n", size, *data);
              switch(*data)
                {
                case Connect:
                  client_connect(&mediator_context, &msg_id, &msg_type, ip_machine);
                  break;

                case Heartbeat:
                  client_heartbeat(&mediator_context, &msg_id, &msg_type, ip_root);
                  break;

                case Data:
                case Debug:
                case Raw:
                  zmq_msg_init(&msg_data);
                  size = zmq_msg_recv(&msg_data,ip_root,0);
                  printf("mediator: third frame was %i bytes\n", size);
                  hasData = true;
                  zmq_msg_t msg_id_copy;
                  zmq_msg_t msg_type_copy;
                  zmq_msg_t msg_data_copy;

                  zmq_msg_init(&msg_id_copy);
                  zmq_msg_init(&msg_type_copy);
                  zmq_msg_init(&msg_data_copy);
                  
                  zmq_msg_copy(&msg_id_copy,&msg_id);
                  zmq_msg_copy(&msg_type_copy,&msg_type);
                  zmq_msg_copy(&msg_data_copy,&msg_data);
                  
                  zmq_msg_send(&msg_id_copy, ip_machine, ZMQ_SNDMORE);
                  zmq_msg_send(&msg_type_copy, ip_machine, ZMQ_SNDMORE);
                  zmq_msg_send(&msg_data_copy, ip_machine, 0);      

                  break;

                default:
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
      else if (items [1].revents & ZMQ_POLLIN)
        {
          //for nwo all messages are forwarded on to clients.

          bool more = false;
          size_t optLen = sizeof(more);
          while(1)
            {
              //              printf("!\n");
              zmq_msg_t msg;
              zmq_msg_init(&msg);
              zmq_msg_recv(&msg, ip_machine,0);
              zmq_msg_t cpy;
              zmq_msg_init(&cpy);
              zmq_msg_copy(&cpy,&msg);
              zmq_msg_close(&msg);
              zmq_getsockopt(ip_machine,ZMQ_RCVMORE,&more, &optLen);
              zmq_msg_send(&cpy,ip_root,more ? ZMQ_SNDMORE : 0);
              if(!more)
                {
                  break;
                }
            }

        }
    }
  return 0;
}

  

  
