DMX_INC =  -I/opt/vc/include/ -I /opt/vc/include/interface/vmcs_host/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
EGL_INC = 
OMX_INC =  -I /opt/vc/include/IL
OMX_ILCLIENT_INC = -I/opt/vc/src/hello_pi/libs/ilclient

MY_INC = -I ../libs/

INCLUDES = $(DMX_INC) $(EGL_INC) $(OMX_INC) $(OMX_ILCLIENT_INC) $(MY_INC)

CFLAGS=-g -DRASPBERRY_PI -DOMX_SKIP64BIT $(INCLUDES)
CPPFLAGS = -march=armv7-a -mtune=cortex-a7

DMX_LIBS =  -L/opt/vc/lib/ -lbcm_host -lvcos -lvchiq_arm -lpthread
EGL_LIBS = -L/opt/vc/lib/ -lEGL -lGLESv2 
OMX_LIBS = -lopenmaxil 
OMX_ILCLIENT_LIBS = -L/opt/vc/src/hello_pi/libs/ilclient -lilclient

MY_LIBS = -L../libs

LDFLAGS =  $(DMX_LIBS) $(EGL_LIBS) $(OMX_LIBS) $(OMX_ILCLIENT_LIBS) $(MY_LIBS)

remote_cam : remote_cam.o print_OMX.o
	cc -o remote_cam $(CFLAGS) remote_cam.o print_OMX.o $(LDFLAGS)

remote_cam.o : 
	cc -c $(CFLAGS) remote_cam.c $(LDFLAGS)

print_OMX.o :
	cc -c $(CFLAGS) ../libs/print_OMX.c $(LDFLAGS) 

clean :
	rm remote_cam remote_cam.o print_OMX.o 