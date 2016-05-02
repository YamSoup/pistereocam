# might need to include and link socket helper depending on what is needed

#if problems with the newer pi possibly remove the -DRASPBERRYPI

DMX_INC =  -I/opt/vc/include/ -I /opt/vc/include/interface/vmcs_host/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
EGL_INC = 
OMX_INC =  -I /opt/vc/include/IL

INCLUDES = $(DMX_INC) $(EGL_INC) $(OMX_INC) $(OMX_ILCLIENT_INC)

CFLAGS=-g -DRASPBERRY_PI -DOMX_SKIP64BIT $(INCLUDES)
CPPFLAGS = -march=armv7-a -mtune=cortex-a7

DMX_LIBS =  -L/opt/vc/lib/ -lbcm_host -lvcos -lvchiq_arm -lpthread
EGL_LIBS = -L/opt/vc/lib/ -lEGL -lGLESv2 
OMX_LIBS = -lopenmaxil 
OMX_ILCLIENT_LIBS = -L/opt/vc/src/hello_pi/libs/ilclient -lilclient

LDFLAGS =  $(DMX_LIBS) $(EGL_LIBS) $(OMX_LIBS) $(OMX_ILCLIENT_LIBS)

rcam.o: rcam.c rcam.h
	cc -c rcam.c $(CFLAGS)

clean :
	rm rcam.o