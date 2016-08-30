DMX_INC =  -I/opt/vc/include/ -I /opt/vc/include/interface/vmcs_host/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
EGL_INC = 
OMX_INC =  -I /opt/vc/include/IL
OMX_ILCLIENT_INC = -I/opt/vc/src/hello_pi/libs/ilclient

MY_INC = -I ../libs/print_OMX/ -I ../libs/socket_helper/ -I ../libs/rcam/

INCLUDES = $(DMX_INC) $(EGL_INC) $(OMX_INC) $(OMX_ILCLIENT_INC) $(MY_INC)

CFLAGS=-g -DRASPBERRY_PI -DOMX_SKIP64BIT -pthread $(INCLUDES)
CPPFLAGS = -march=armv7-a -mtune=cortex-a7

DMX_LIBS =  -L/opt/vc/lib/ -lbcm_host -lvcos -lvchiq_arm -lpthread
EGL_LIBS = -L/opt/vc/lib/ -lEGL -lGLESv2 
OMX_LIBS = -lopenmaxil 
OMX_ILCLIENT_LIBS = -L/opt/vc/src/hello_pi/libs/ilclient -lilclient
#possibly eventally creat my of liabries out of the .o
#(using ar libares must be in the format lib*.a then added like ilclient above currently they are just)
MY_LIBS = 

LDFLAGS =  $(DMX_LIBS) $(EGL_LIBS) $(OMX_LIBS) $(OMX_ILCLIENT_LIBS) $(MY_LIBS)

remote_cam : remote_cam.o
	cc -o remote_cam remote_cam.o $(LDFLAGS) ../libs/print_OMX/print_OMX.o ../libs/socket_helper/socket_helper.o ../libs/rcam/rcam.o

remote_cam.o : remote_cam.c
	cc -c remote_cam.c $(CFLAGS)

clean :
	rm remote_cam remote_cam.o
