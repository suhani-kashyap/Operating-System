CC=gcc
CFLAGS=-DFRAME_STORE_SIZE=$(framesize) -DVAR_STORE_SIZE=$(varmemsize)

mysh: shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -D FRAME_STORE_SIZE=$(framesize) -D VAR_STORE_SIZE=$(varmemsize) -c shellmemory.c shell.c interpreter.c
	$(CC) $(CFLAGS) -o mysh shellmemory.o shell.o interpreter.o 
clean: 
	rm mysh; rm *.o
