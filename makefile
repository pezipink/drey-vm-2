CC = cl

memory: drey_main.c
    $(CC) -Zi drey_main.c memory\fixed_pool.c memory\dynamic_pool.c