#include "rcam.h"

#include "bcm_host.h"
#include "ilclient.h"

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "print_OMX.h"
#include "socket_helper.h"

/*
This function creates a renderer on the server and comunicates with rcam client program to
display a preview until stopped
Somehow needs to take a photo as well?
not really sure how to control once started?
unsure if this is possible - possibly by creating a global context to control the loop??

as a function that needs to be a thread this can only have 1 argument
and so will need a pointer to a struct containing all the needed parameters.
the struct should go into a header.

also a "global variable" will be needed to control the camera and mutex to control the the variable from both sides
either that or possibly a semephore to control the state

The display type selects if the preview runs full screen. or on the left or right side of a side by side.
*/
int initServerRcam(void *VoidPtrArgs)
{    
  struct cameraControl *currentArgs = VoidPtrArgs;

  pthread_mutex_lock(&currentArgs->mutexPtr);  
  ILCLIENT_T *client = currentArgs->client;
  pthread_mutex_unlock(&currentArgs->mutexPtr);

  ///////////////////////////////////////////
  ////Variables
  ///////////////////////////////////////////

  COMPONENT_T *client_video_render = NULL;
  OMX_ERRORTYPE OMXstatus;

  OMX_BUFFERHEADERTYPE *client_video_render_in;

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

  rcam_command rcamToSend = NO_COMMAND;

  /////////////////////////////////////////////////////////////////
  // SOCKET STUFF
  /////////////////////////////////////////////////////////////////
  printf("start of socket stuff in rcam\n");

  int socket_fd = 0, client_socket_fd = 0;
  socket_fd = getAndBindTCPServerSocket(PORT);
  
  printf("waiting for remotecam to connect\n");

  client_socket_fd = listenAndAcceptTCPServerSocket(socket_fd, 10/*backlog*/);

  printf("socket_fd = %d\n", socket_fd);
  printf("client_socket_fd = %d\n", client_socket_fd);


  ///////////////////////////////////////////
  ////Initialise client video render
  ///////////////////////////////////////////
  pthread_mutex_lock(&currentArgs->mutexPtr);

  ilclient_create_component(client,
			    &client_video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_INPUT_BUFFERS);

  OMXstatus = ilclient_change_component_state(client_video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move render component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }

  //set the port params to the same as remoteCam.c

  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  render_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  render_params.format.video.nFrameWidth = currentArgs->previewWidth;
  render_params.format.video.nFrameHeight = currentArgs->previewHeight;
  render_params.format.video.nStride = currentArgs->previewWidth;
  render_params.format.video.nSliceHeight = currentArgs->previewHeight;
  render_params.format.video.xFramerate = 24 << 16;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(client_video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting video render port parameters (1)");

  pthread_mutex_unlock(&currentArgs->mutexPtr);
  
  //check the port params
  memset(&render_params, 0, sizeof(render_params));
  render_params.nVersion.nVersion = OMX_VERSION;
  render_params.nSize = sizeof(render_params);
  render_params.nPortIndex = 90;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  print_OMX_PARAM_PORTDEFINITIONTYPE(render_params);

  //set the position on the screen
  pthread_mutex_lock(&currentArgs->mutexPtr);  

  render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
					   |OMX_DISPLAY_SET_FULLSCREEN
					   |OMX_DISPLAY_SET_NOASPECT
					   |OMX_DISPLAY_SET_MODE);
  render_config.fullscreen = OMX_FALSE;
  render_config.noaspect = OMX_FALSE;

  render_config.dest_rect.width = currentArgs->screenWidth/2;
  render_config.dest_rect.height = currentArgs->screenHeight;
  render_config.dest_rect.x_offset = currentArgs->screenWidth/2;

  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(client_video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));

  pthread_mutex_unlock(&currentArgs->mutexPtr);  

  //ask ilclient to allocate buffers for client_video_render
  pthread_mutex_lock(&currentArgs->mutexPtr);  

  printf("enable client_video_render_input port\n");
  ilclient_enable_port_buffers(client_video_render, 90, NULL, NULL,  NULL);
  ilclient_enable_port(client_video_render, 90);

  pthread_mutex_unlock(&currentArgs->mutexPtr);  
  /*
  DOES NOT WORK LEAVING AS A REMINDER

  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90;
  render_config.set = OMX_DISPLAY_SET_DUMMY;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Config. Error = %s\n", err2str(OMXstatus));
  print_OMX_CONFIG_DISPLAYREGIONTYPE(render_config);
  */

  //change preview render to executing
  pthread_mutex_lock(&currentArgs->mutexPtr);
  OMXstatus = ilclient_change_component_state(client_video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printf("client_video_render state is ");
  printState(ilclient_get_handle(client_video_render));
  printf("***\n");

  pthread_mutex_unlock(&currentArgs->mutexPtr);  

  ////////////////////////////////////////////////////////////
  // SEND AND RECV
  ////////////////////////////////////////////////////////////

  //handshake
  printf("waiting to recive handshake ... \n");
  read(client_socket_fd, char_buffer, 11);
  printf("handshake result = %s", char_buffer);
  write(client_socket_fd, "got\0", sizeof(char)*4);

  void * temp_buffer;
  temp_buffer = malloc(render_params.nBufferSize + 1 );

  int count = 0;
  long int num_bytes = 0;
  enum rcam_command current_command = START_PREVIEW;

  printf("current_command = %d\n", current_command);

  printf("sending command ...");
  write(client_socket_fd, &current_command, sizeof(current_command));
  printf("sent command\n");

  current_command = NO_COMMAND;

  printf("*** nBufferSize = %d\n", render_params.nBufferSize);

  ////////////////////////////////////////////////////////////
  //// Main Thread Loop
  ////////////////////////////////////////////////////////////

  bool rcamLoopEsc = false;

  //modify to inifinate loop when control functions are writen
  while(1)
    {
      //escape condition needs mutex so is not in while condition
      pthread_mutex_lock(&currentArgs->mutexPtr);
      rcamLoopEsc = currentArgs->rcamDeInit;
      pthread_mutex_unlock(&currentArgs->mutexPtr);  
      if(rcamLoopEsc != false) break; //exits while loop
     

      printState(ilclient_get_handle(client_video_render));
      count++;
      printf("count = %d\n", count);

      printf("get a buffer to process\n");
      printf("waiting to recv buffer of size %d... ", render_params.nBufferSize);
      num_bytes = read(client_socket_fd,
		       temp_buffer,
		       render_params.nBufferSize);
      while (num_bytes < render_params.nBufferSize)
	{
	  num_bytes += read(client_socket_fd,
			    temp_buffer + num_bytes,
			    render_params.nBufferSize - num_bytes);
	}
      printf("buffer recived, recived %ld bytes\n", num_bytes);

      //change nAllocLen in bufferheader
      client_video_render_in = ilclient_get_input_buffer(client_video_render, 90, 1);
      memcpy(client_video_render_in->pBuffer, temp_buffer, render_params.nBufferSize);
      printf("copied buffer form temp into client_video_render_in\n");
      //fix alloc len
      client_video_render_in->nFilledLen = render_params.nBufferSize;

      //empty buffer into render component
      OMX_EmptyThisBuffer(ilclient_get_handle(client_video_render), client_video_render_in);
      printf("Emptied buffer\n");

      //send no command
      write(client_socket_fd, &current_command, sizeof(current_command));
    }


  putchar('\n');

  ////////////////////////////////////////////////////////////
  //// end of thread
  ////////////////////////////////////////////////////////////
   
  //free buffer memory
  free(temp_buffer);

  //!free ilobjects and make sure all allocated memory is free!

  //!free sockets try to ensure no zombies

}

void deInitServerRcam(struct cameraControl *toChange)
{
  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->rcamDeInit = 1;
  pthread_mutex_unlock(&toChange->mutexPtr);
}

// does not need to have void pointer attribute as will run in main thread
// This is a template Function for all Functions that will control rcam like take photo
// this will be achived by changing shared memory to control the loop in initServerRcam

/*
int exampleVariableChanger (struct cameraControl *toChange)
{
  //does not need to be &mutexPtr as is passed as a pointer
  // this is an atempt to not declare it globally
  pthread_mutex_lock(&toChange->mutexPtr); //check this would actually work
  toChange->variable = value;
  pthread_mutex_unlock(&toChange->mutexPtr);
}
*/
