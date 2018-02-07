CC = cl

drey: drey_main.c
    $(CC) -Zi drey_main.c memory\fixed_pool.c memory\dynamic_pool.c memory\manager.c datastructs\refstack.c datastructs\refhash.c  /DDEBUG


drey_test: drey_test.c
    $(CC) -Zi drey_test.c memory\fixed_pool.c memory\dynamic_pool.c memory\manager.c datastructs\refstack.c datastructs\refhash.c 