#include <winsock2.h>
#include <windows.h>
#include "machine.h"
#include <time.h>
#include "..\zmq.h"
#include "..\global.h"
#include "..\zhelpers.h"
#include "..\vm\vm.h"
#include "..\vm\debugger.h"
#include "..\memory\gc.h"
#include "..\datastructs\refhash.h"
#include "..\datastructs\refarray.h"
#include "..\datastructs\json.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>



DWORD WINAPI machine_thread(LPVOID context)
{
    printf("machine thread created context at %p\n", context);
    void* ip_machine = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(ip_machine, "inproc://machine");
    printf("machine signalling mediator\n");

    vm machine = init_vm(ip_machine);
    s_send(ip_machine, "READY");

    zmq_pollitem_t items[] = {
      { ip_machine, 0, ZMQ_POLLIN, 0 }
    };

    int count = 0;
    while (1)
    {
        zmq_poll(items, 1, 1);
        if (items[0].revents & ZMQ_POLLIN)
        {
            zmq_msg_t msg_id;
            zmq_msg_t msg_type;
            zmq_msg_t msg_data;
            int id_size = 0;
            int size = 0;
            zmq_msg_init(&msg_id);
            id_size = zmq_msg_recv(&msg_id, ip_machine, 0);

            bool more = false;
            bool hasData = false;
            bool hasType = false;
            size_t optLen = sizeof(more);
            enum MessageType type;
            zmq_getsockopt(ip_machine, ZMQ_RCVMORE, &more, &optLen);
            if (more) //this must be more
            {
                zmq_msg_init(&msg_type);
                size = zmq_msg_recv(&msg_type, ip_machine, 0);
                hasType = true;
                char* data = zmq_msg_data(&msg_type);
                //              printf("mediator: second frame was %i bytes with value %i\n", size, *data);
                switch (*data)
                {
                case Connect:
                    vm_client_connect(&machine, zmq_msg_data(&msg_id), id_size);
                    break;

                case Data:
                case Debug:
                    zmq_msg_init(&msg_data);
                    size = zmq_msg_recv(&msg_data, ip_machine, 0);
                    //extract response.
                    char *json_data = zmq_msg_data(&msg_data);
                    memref json = json_to_object(&json_data, size);
                    if (*data == Data)
                    {
                        memref id_val = hash_try_get_string_key(json, "id");
                        memref clientid = ra_init_str_raw(zmq_msg_data(&msg_id), id_size);
                        if (id_val.type != 0)
                        {
                            vm_handle_response(&machine, clientid, id_val);
                        }
                        else
                        {
                            printf("did not understand memssage from client\n");
                        }
                    }
                    else
                    {
                        printf("debug message received\n");
                        //print_hash(json,0);
                        if (json.type == 0)
                        {
                            printf("CRITICAL WARNING, RECIVED BLANK JSON MESSAGE!\n");
                            break;
                        }
                        int response = handle_debug_msg(json, &machine);
                        if (response >= 1)
                        {
                            zmq_msg_t msg_id_copy;
                            zmq_msg_t msg_type_copy;
                            zmq_msg_t msg_data_new;
                            zmq_msg_init(&msg_id_copy);
                            zmq_msg_init(&msg_type_copy);
                            zmq_msg_copy(&msg_id_copy, &msg_id);
                            zmq_msg_copy(&msg_type_copy, &msg_type);

                            stringref json_str = object_to_json(json);
                            //ra_wl(json_str);
                            msg_data_new = ra_to_msg(json_str);
                            zmq_msg_send(&msg_id_copy, ip_machine, ZMQ_SNDMORE);
                            zmq_msg_send(&msg_type_copy, ip_machine, ZMQ_SNDMORE);
                            zmq_msg_send(&msg_data_new, ip_machine, 0);

                            zmq_msg_close(&msg_id_copy);
                            zmq_msg_close(&msg_type_copy);
                            zmq_msg_close(&msg_data_new);
                        }

                        if (response >= 2)
                        {
                            zmq_msg_t msg_id_copy;
                            zmq_msg_t msg_type_copy;
                            zmq_msg_t msg_data_new;
                            zmq_msg_init(&msg_id_copy);
                            zmq_msg_init(&msg_type_copy);
                            zmq_msg_copy(&msg_id_copy, &msg_id);
                            zmq_msg_copy(&msg_type_copy, &msg_type);

                            memref new_type = build_general_announce(&machine);
                            stringref json_str = object_to_json(new_type);
                            //ra_wl(json_str);
                            msg_data_new = ra_to_msg(json_str);
                            zmq_msg_send(&msg_id_copy, ip_machine, ZMQ_SNDMORE);
                            zmq_msg_send(&msg_type_copy, ip_machine, ZMQ_SNDMORE);
                            zmq_msg_send(&msg_data_new, ip_machine, 0);

                            zmq_msg_close(&msg_id_copy);
                            zmq_msg_close(&msg_type_copy);
                            zmq_msg_close(&msg_data_new);
                        }

                    }

                    gc_mark_n_sweep(&machine);
                    hasData = true;

                    break;
                case Raw:
                    zmq_msg_init(&msg_data);
                    size = zmq_msg_recv(&msg_data, ip_machine, 0);
                    vm_handle_raw(&machine, zmq_msg_data(&msg_id), id_size, zmq_msg_data(&msg_data), zmq_msg_size(&msg_data));
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

            if (hasType)
            {
                zmq_msg_close(&msg_type);
            }

            if (hasData)
            {
                int x = zmq_msg_close(&msg_data);
            }

            zmq_msg_close(&msg_id);

        }
        else if (!machine.game_over && machine.num_players == machine.req_players)
        {
            //debug
            if (machine.debugger_connected == 0)
            {
                vm_run(&machine);
            }
            //todo: compress dynamic memory
        }
    }
}


