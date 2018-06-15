CC = cl

drey: drey_main.c
    $(CC) /Zi zmq.lib drey_main.c threads\mediator.c threads\machine.c vm\vm.c memory\gc.c memory\fixed_pool.c memory\dynamic_pool.c memory\manager.c  datastructs\reflist.c datastructs\refstack.c datastructs\refhash.c  datastructs\json.c  datastructs\refarray.c  /DDEBUG

drey_test: drey_test.c
    $(CC) /Zi drey_test.c memory\fixed_pool.c memory\dynamic_pool.c memory\manager.c  datastructs\reflist.c datastructs\refstack.c datastructs\refhash.c datastructs\refarray.c datastructs\json.c /DDEBUG

drey_test_r: drey_test.c
    $(CC) /Ox zmq.lib drey_test.c threads\mediator.c threads\machine.c vm\vm.c memory\fixed_pool.c memory\dynamic_pool.c memory\manager.c memory\gc.c datastructs\refstack.c datastructs\refhash.c  datastructs\json.c datastructs\refarray.c 
