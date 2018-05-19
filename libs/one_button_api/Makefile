#assuming the includes in the file dont need linked in recipy?
#if problems with the newer pi possibly remove the -DRASPBERRYPI

CFLAGS = #-g -DRASPBERRY_PI -DOMX_SKIP64BIT

socket_helper.o : one_button_api.c one_button_api.h
	cc -c one_button_api.c $(CFLAGS) -pthread -lwiringPi

clean :
	rm socket_helper.o
