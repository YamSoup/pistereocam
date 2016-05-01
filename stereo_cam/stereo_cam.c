/*
    use ilclient_GetParameter, ilclient_SetParameter to setup components before executing state
    use ilclient_GetConfig and ilclient_SetConfig to change settings while in executing state

    some settings can be changed before executing and some while executing

    all 4 functions use OMX_INDEXTYPE enumeration to specify what settings are being changed

    for index for each component look at
    http://www.jvcref.com/files/PI/documentation/ilcomponents/index.html

    for relevent data stuctures see
    http://maemo.org/api_refs/5.0/beta/libomxil-bellagio/_o_m_x___index_8h.html
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "print_OMX.h"
#include "socket_helper.h"
#include "rcam.h"

//defines
#define PORT "8039"

/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data);

/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  printf("sterocam started\n");
  /////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////

  ILCLIENT_T *client;
  COMPONENT_T *video_render = NULL;

  OMX_ERRORTYPE OMXstatus;

  uint32_t screen_width = 0, screen_height = 0;
  OMX_BUFFERHEADERTYPE *video_render_in;

  int numbytes;
  char char_buffer[12];

  OMX_PARAM_PORTDEFINITIONTYPE render_params;
  memset(&render_params, 0, sizeof(render_params));
  render_params.nVersion.nVersion = OMX_VERSION;
  render_params.nSize = sizeof(render_params);
  render_params.nPortIndex = 90;

  OMX_CONFIG_DISPLAYREGIONTYPE render_config;
  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90;

  /////////////////////////////////////////////////////////////////
  // SOCKET STUFF
  /////////////////////////////////////////////////////////////////
  printf("start of socket stuff");
  
  //socket stuctures and vars and stuff
  int socket_fd, new_sock;
  struct addrinfo hints, *results;
  struct sockaddr_storage remote_cam_addr;
  socklen_t addr_size = sizeof(remote_cam_addr);
  int socket_status = 0, result;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // getaddrinfo
  socket_status = getaddrinfo(NULL, PORT, &hints, &results);
  if(socket_status != 0)
    {
      fprintf(stderr, "getaddrinfo failed, error = %s\n", gai_strerror(socket_status));
      exit(EXIT_FAILURE);
    }

  printf("results->ai_protocol = %d", results->ai_protocol);

  // socket
  socket_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(socket_fd <= 0)
    {
      fprintf(stderr, "socket failed\n");
      exit(EXIT_FAILURE);
    }

  // bind
  socket_status = bind(socket_fd, results->ai_addr, results->ai_addrlen);
  if(socket_status != 0)
    {
      fprintf(stderr, "bind failed\n");
      exit(EXIT_FAILURE);
    }

  freeaddrinfo(results);

  printf("waiting for remotecam to connect");
  
  //listen
  if (listen(socket_fd, 10) == -1)
    {
      fprintf(stderr, "listen failed");
      exit(EXIT_FAILURE);
    }

  //accept
  new_sock = accept(socket_fd, (struct sockaddr *) &remote_cam_addr, &addr_size); 
  if(new_sock < 0)
    printf("error accepting socket, error = %s", strerror(errno));

  printf("socket = %d\n", socket_fd);
  printf("new_sock = %d", new_sock);
  /////////////////////////////////////////////////////////////////
  // FORK
  /////////////////////////////////////////////////////////////////
#ifdef FORK

  pid_t pid;

  pid = fork();
  if (pid == 0)
    printf("in child process!!!\n");  //call a fuction etc
  if (pid < 0)
    fprintf(stderr, "Fork failed!\n");
  else /* if pid > 0 */
    printf("in main process\n");

#endif

  /////////////////////////////////////////////////////////////////
  // STARTUP
  /////////////////////////////////////////////////////////////////

  //initialise bcm_host
  bcm_host_init();

  //create client
  client = ilclient_init();
  if(client == NULL)
    {
      fprintf(stderr, "unable to initalize ilclient\n");
      exit(EXIT_FAILURE);
    }

  //initalize OMX
  OMXstatus = OMX_Init();
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to initalize OMX");
      ilclient_destroy(client);
      exit(EXIT_FAILURE);
    }

  //set error callback
  ilclient_set_error_callback(client,
			      error_callback,
			      NULL);

  //super special broadcom only function
  graphics_get_display_size(0/*framebuffer 0*/, &screen_width, &screen_height);
  printf("screen_width = %d, screen_height = %d\n", (int)screen_width, (int)screen_height);


  /////////////////////////////////////////////////////////////////
  // Initalize Components
  /////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////
  ////Initialise video render////
  ///////////////////////////////////////////
  ilclient_create_component(client,
			    &video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_INPUT_BUFFERS);

  OMXstatus = ilclient_change_component_state(video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move render component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }

  //set the port params to the same as the remote camera

  int width = 320, height = 240;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  render_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  render_params.format.video.nFrameWidth = width;
  render_params.format.video.nFrameHeight = height;
  render_params.format.video.nStride = width;
  render_params.format.video.nSliceHeight = height;
  render_params.format.video.xFramerate = 24 << 16;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting video render port parameters (1)");

  //check the port params
  memset(&render_params, 0, sizeof(render_params));
  render_params.nVersion.nVersion = OMX_VERSION;
  render_params.nSize = sizeof(render_params);
  render_params.nPortIndex = 90;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  print_OMX_PARAM_PORTDEFINITIONTYPE(render_params);

  //set the position on the screen
  /*
  render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
					   |OMX_DISPLAY_SET_FULLSCREEN
					   |OMX_DISPLAY_SET_NOASPECT
					   |OMX_DISPLAY_SET_MODE);
  render_config.fullscreen = OMX_FALSE;
  render_config.noaspect = OMX_FALSE;

  render_config.dest_rect.width = screen_width/2;
  render_config.dest_rect.height = screen_height;

  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));
  */

  //ask ilclient to allocate buffers for video_render
  printf("enable video_render_input port\n");
  ilclient_enable_port_buffers(video_render, 90, NULL, NULL,  NULL);
  ilclient_enable_port(video_render, 90);


  /*
  DOES NOT WORK LEAVING AS A REMINDER

  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90;
  render_config.set = OMX_DISPLAY_SET_DUMMY;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Config. Error = %s\n", err2str(OMXstatus));
  print_OMX_CONFIG_DISPLAYREGIONTYPE(render_config);
  */


  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////

  //change preview render to executing
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printf("video_render state is ");
  printState(ilclient_get_handle(video_render));
  printf("***\n");

  ////////////////////////////////////////////////////////////
  // SEND AND RECV
  ////////////////////////////////////////////////////////////
  
  //handshake
  printf("waiting to recive handshake ... \n");
  read(new_sock, char_buffer, 11);
  printf("handshake result = %s", char_buffer);
  write(new_sock, "got\0", sizeof(char)*4);

  void * temp_buffer;
  temp_buffer = malloc(render_params.nBufferSize + 1 );  

  int count = 0;
  long int num_bytes = 0;
  enum rcam_command current_command = START_PREVIEW; 

  printf("current_command = %d\n", current_command);

  printf("sending command ...");
  write(new_sock, &current_command, sizeof(current_command));
  printf("sent command\n");
  
  current_command = NO_COMMAND;

  printf("*** nBufferSize = %d\n", render_params.nBufferSize);

  while(count < 100)
    {
      printState(ilclient_get_handle(video_render));
      count++;
      printf("count = %d\n", count);

      printf("get a buffer to process\n");
      printf("waiting to recv buffer of size %d... ", render_params.nBufferSize);
      num_bytes = read(new_sock,
		       temp_buffer,
		       render_params.nBufferSize);
      while (num_bytes < render_params.nBufferSize)
	{
	  num_bytes += read(new_sock,
			    temp_buffer + num_bytes,
			    render_params.nBufferSize - num_bytes);
	}
      printf("buffer recived, recived %ld bytes\n", num_bytes);
      
      //change nAllocLen in bufferheader
      video_render_in = ilclient_get_input_buffer(video_render, 90, 1);
      memcpy(video_render_in->pBuffer, temp_buffer, render_params.nBufferSize);
      printf("copied buffer form temp into video_render_in\n");
      //fix alloc len
      video_render_in->nFilledLen = render_params.nBufferSize;

      //empty buffer into render component
      OMX_EmptyThisBuffer(ilclient_get_handle(video_render), video_render_in);
      printf("Emptied buffer\n");

      //send no command
      write(new_sock, &current_command, sizeof(current_command));      
    }
      
  
  putchar('\n');

  //sleep for 2 secs
  sleep(2);


  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //free buffer memory
  free(temp_buffer);
  
  //Disable components


  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}
