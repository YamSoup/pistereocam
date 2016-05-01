#assuming the includes in the file dont need linked in recipy?
#if problems with the newer pi possibly remove the -DRASPBERRYPI

CFLAGS = #-g -DRASPBERRY_PI -DOMX_SKIP64BIT

socket_helper.o : socket_helper.c socket_helper.h
	cc -c socket_helper.c $(CFLAGS) -lm

clean :
	rm socket_helper.o