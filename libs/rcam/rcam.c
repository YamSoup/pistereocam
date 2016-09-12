#include "rcam.h"

#include "bcm_host.h"
#include "ilclient.h"

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "print_OMX.h"
#include "socket_helper.h"

#define PORT "8039"

/*
to be writen and tested in camera
The task is to make a thread version of the existing camera.c
next task will be to simplify the initServerRcam using the same functions
 */

void *initLocalCamera(void *VoidPtrArgs)
{
  struct cameraControl *currentArgs = VoidPtrArgs;

  pthread_mutex_lock(&currentArgs->mutexPtr);  
  ILCLIENT_T *client = currentArgs->client;
  pthread_mutex_unlock(&currentArgs->mutexPtr);
  
  COMPONENT_T *camera = NULL, *video_render = NULL, *image_encode = NULL;
  OMX_ERRORTYPE OMXstatus;

  FILE *file_out1;
  file_out1 = fopen("pic1", "wb");
  
  TUNNEL_T tunnel_camera_to_render, tunnel_camera_to_encode;
  memset(&tunnel_camera_to_render, 0, sizeof(tunnel_camera_to_render));
  memset(&tunnel_camera_to_encode, 0, sizeof(tunnel_camera_to_encode));  
    
  /////////////////////////////////////////////////////////////////
  // STARTUP
  /////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////
  //
  // Initalize Components
  //
  /////////////////////////////////////////////////////////////////
  
  ///////////////////////////////////////////
  ////initialise camera////
  ///////////////////////////////////////////
  ilclient_create_component(client,
			    &camera,
			    "camera",
			    ILCLIENT_DISABLE_ALL_PORTS);
  
  OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Idle (1)");
      exit(EXIT_FAILURE);
    }

  //change the capture resolution
  setCaptureRes(camera, currentArgs->photoWidth, currentArgs->photoHeight);

  //change the preview resolution
  setPreviewRes(camera, currentArgs->previewWidth, currentArgs->previewHeight);
  
  ///////////////////////////////////////////
  ////Initialise video render////
  ///////////////////////////////////////////

  ilclient_create_component(client,
			    &video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS
			    );

  OMXstatus = ilclient_change_component_state(video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move render component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }

  setRenderConfig(video_render, currentArgs->displayType);  

  ///////////////////////////////////////////
  ////Initalise Image Encoder///
  ///////////////////////////////////////////
  ilclient_create_component(client,
			    &image_encode,
			    "image_encode",
			    ILCLIENT_DISABLE_ALL_PORTS
			    /*| ILCLIENT_ENABLE_INPUT_BUFFERS*/
			    | ILCLIENT_ENABLE_OUTPUT_BUFFERS);

  OMXstatus = ilclient_change_component_state(image_encode, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move image encode component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }

  //image format Param set */
  setParamImageFormat(image_encode, JPEG_HIGH_FORMAT);
   
  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////

  //setup tunnel of camera preview to renderer
  set_tunnel(&tunnel_camera_to_render, camera, 70, video_render, 90);
  ilclient_setup_tunnel(&tunnel_camera_to_render, 0, 0);
  
  // change camera component to executing
  OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  
  //change preview render to executing
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
    
  //enable port and buffers for output port of image encode
  ilclient_enable_port_buffers(image_encode, 341, NULL, NULL, NULL);  
  ilclient_enable_port(image_encode, 341);

  //setup tunnel from camera image port too image encode
  set_tunnel(&tunnel_camera_to_encode, camera, 72, image_encode, 340);    
  ilclient_setup_tunnel(&tunnel_camera_to_encode, 0, 0);

  //change image_encode to executing
  OMXstatus = ilclient_change_component_state(image_encode, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move image_encode component to Executing (1) Error = %s\n", err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }
  
  //////////////////////////////////////////////////////
  // Code that takes picture
  //////////////////////////////////////////////////////

  //needs a lot of work!
  
  while(1)
    {
      pthread_mutex_lock(&currentArgs->mutexPtr);  
      if (currentArgs->rcamDeInit == true)
	{
	  printf("end\n");
	  pthread_mutex_unlock(&currentArgs->mutexPtr);
	  break;
	}
      else if (currentArgs->takePhoto == true)
	{
	  savePhoto(camera, image_encode, file_out1);
	  currentArgs->takePhoto = false;
	}
      pthread_mutex_unlock(&currentArgs->mutexPtr);
      
    }
  
  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //close files
  fclose(file_out1);
  
  //Disable components
  pthread_exit(NULL);  

  //check all components have been cleaned up  
}


/*
*********************************************
remote camera
*********************************************
This function creates a renderer on the server and comunicates with rcam client program to
display a preview until stopped
Somehow needs to take a photo as well?

Idea for camera capture on rcam 
Create a buffer large enough to hold the biggest image possible and an int for the size of the final buffer
Transfer the whole image when it is finished 
This will get over the issue of having to wait for the flag to tell when the process has stopped !

as a function that needs to be a thread this can only have 1 argument
and so will need a pointer to a struct containing all the needed parameters.
the struct should go into a header.

also a "global variable" will be needed to control the camera and mutex to control the the variable from both sides
either that or possibly a semephore to control the state

The display type selects if the preview runs full screen. or on the left or right side of a side by side.
*/
void *initServerRcam(void *VoidPtrArgs)
{    
  struct cameraControl *currentArgs = VoidPtrArgs;

  pthread_mutex_lock(&currentArgs->mutexPtr);  
  ILCLIENT_T *client = currentArgs->client;
  pthread_mutex_unlock(&currentArgs->mutexPtr);

  ///////////////////////////////////////////
  ////Variables
 
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

  enum rcam_command rcam_command = NO_COMMAND;

  /////////////////////////////////////////////////////////////////
  // SOCKET STUFF

  printf("start of socket stuff in rcam\n");

  int socket_fd = 0, client_socket_fd = 0;
  
  socket_fd = getAndBindTCPServerSocket(PORT);
  
  printf("waiting for remotecam to connect\n");

  client_socket_fd = listenAndAcceptTCPServerSocket(socket_fd, 10/*backlog*/);

  printf("socket_fd = %d\n", socket_fd);
  printf("client_socket_fd = %d\n", client_socket_fd);
  
  ///////////////////////////////////////////
  ////Initialise client video render

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

  //handshake
  printf("waiting to recive handshake ... \n");
  read(client_socket_fd, char_buffer, 11);
  printf("handshake result = %s", char_buffer);
  write(client_socket_fd, "got\0", sizeof(char)*4);

  void * temp_buffer;
  temp_buffer = malloc(render_params.nBufferSize + 1 );

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

  bool rcamLoopEsc = false;

  //modify to inifinate loop when control functions are writen
  while(1)
    {
      //escape condition needs mutex so is not in while condition
      pthread_mutex_lock(&currentArgs->mutexPtr);
      rcamLoopEsc = currentArgs->rcamDeInit;
      pthread_mutex_unlock(&currentArgs->mutexPtr);  
      
      printState(ilclient_get_handle(client_video_render));
      
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

      
      if(rcamLoopEsc == true)
	{
	  current_command = END_REMOTE_CAM;
	  write(client_socket_fd, &current_command, sizeof(current_command));
  	  printf("END_REMOTE_CAM sent\n");
	  break; //exits while loop
	}

      //send no command
      write(client_socket_fd, &current_command, sizeof(current_command));
    }

  ////////////////////////////////////////////////////////////
  //// end of thread Cleanup
   
  //free buffer memory
  free(temp_buffer);
  printf("temp_buffer memory freed");
  //!free ilobjects and make sure all allocated memory is free!

  //!free sockets try to ensure no zombies
  printf("exiting rcam thread");
  pthread_exit(NULL);
}

void deInitServerRcam(struct cameraControl *toChange)
{
  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->rcamDeInit = 1;
  pthread_mutex_unlock(&toChange->mutexPtr);
}

/////////////////////////////////////////////////////////
// Functions that were originally in camera.c
/////////////////////////////////////////////////////////

//function returns screensize seperated only to make porting easier
struct screenSizeStruct returnScreenSize(void)
{
  struct screenSizeStruct CurrentScreenSize;
  //super special broadcom only function
  graphics_get_display_size(0/*framebuffer 0*/, &CurrentScreenSize.width, &CurrentScreenSize.height);
  return CurrentScreenSize;
}    

//sets the capture resolution of the camera (any camera)
void setCaptureRes(COMPONENT_T *camera, int width, int height)
{
  //needs to check width and height to see if compatible with rpi
  printf("in setCapture\n");
  
  OMX_PARAM_PORTDEFINITIONTYPE port_params;
  OMX_ERRORTYPE OMXstatus;

  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 72;

  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter In setCaptureRes. Error = %s\n", err2str(OMXstatus));
  //change needed params
  port_params.format.image.nFrameWidth = width;
  port_params.format.image.nFrameHeight = height;
  port_params.format.image.nStride = 0; //needed! set to 0 to recalculate
  port_params.format.image.nSliceHeight = 0;  //notneeded?
  //this does not work :( camera seams only to output YUV
  //port_params.format.image.eColorFormat = OMX_COLOR_Format32bitABGR8888;
  
  //set changes
  OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter In setCaptureRes. Error = %s\n", err2str(OMXstatus));

  /*
  //print current config 
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 72;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter (2) In setCaptureRes. Error = %s\n", err2str(OMXstatus));

  print_OMX_PARAM_PORTDEFINITIONTYPE(port_params);
  */
}


//sets the preview res of the camera
void setPreviewRes(COMPONENT_T *camera, int width, int height)
{
  //needs to check width and height to see if compatible with rpi
  printf("in setPreviewRes\n");
  
  OMX_PARAM_PORTDEFINITIONTYPE port_params;
  OMX_ERRORTYPE OMXstatus;

  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 70;
  //prepopulate structure
  OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));
  //change needed params
  port_params.format.video.nFrameWidth = width;
  port_params.format.video.nFrameHeight = height;
  port_params.format.video.nStride = width;
  port_params.format.video.nSliceHeight = height;
  port_params.format.video.xFramerate = 24 << 16;
  //set changes
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));

  /*
  //print current config
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 70;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter (2) In setPreviewRes. Error = %s\n", err2str(OMXstatus));

  print_OMX_PARAM_PORTDEFINITIONTYPE(port_params);      
  */
}

// sets the preview size and position
// takes the enum displayTypes the contents of which should hopefully be self explanitory
void setRenderConfig(COMPONENT_T *video_render, enum display_types presetScreenConfig)
{
  struct screenSizeStruct currentScreenSize;
  currentScreenSize = returnScreenSize();

  int screenWidth = currentScreenSize.width; 
  int screenHeight = currentScreenSize.height;
  
  printf("in setRenderConfig\n");
  OMX_ERRORTYPE OMXstatus;
  
  OMX_CONFIG_DISPLAYREGIONTYPE render_config;
  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90;
  
  //curently only does fullscreen modify the stucture below with switch/if statements for the others
  render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
					   |OMX_DISPLAY_SET_FULLSCREEN
					   |OMX_DISPLAY_SET_NOASPECT
    					   |OMX_DISPLAY_SET_MODE);
  //FULLSCREEN
  if(presetScreenConfig == DISPLAY_FULLSCREEN)
    {
      render_config.dest_rect.width = screenWidth;
      render_config.dest_rect.height = screenHeight;
      render_config.dest_rect.x_offset = 0;
      render_config.dest_rect.y_offset = 0;
    }
  //SIDEBYSIDE
  else if(presetScreenConfig == DISPLAY_SIDEBYSIDE_LEFT)
    {
      render_config.dest_rect.width = screenWidth/2;
      render_config.dest_rect.height = screenHeight;
      render_config.dest_rect.x_offset = 0;
      render_config.dest_rect.y_offset = 0;
    }
  else if(presetScreenConfig == DISPLAY_SIDEBYSIDE_RIGHT)
    {
      render_config.dest_rect.width = screenWidth/2;
      render_config.dest_rect.height = screenHeight;
      render_config.dest_rect.x_offset = screenWidth/2;
      render_config.dest_rect.y_offset = 0;
    }
  //QUARTER
  else if(presetScreenConfig == DISPLAY_QUARTER_TOP_LEFT)
    {
      render_config.dest_rect.width = screenWidth/2;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = 0;
      render_config.dest_rect.y_offset = 0;
    }
  else if(presetScreenConfig == DISPLAY_QUARTER_TOP_RIGHT)
    {
      render_config.dest_rect.width = screenWidth/2;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = screenWidth/2;;
      render_config.dest_rect.y_offset = 0;
    }
  else if(presetScreenConfig == DISPLAY_QUARTER_BOTTOM_LEFT)
    {
      render_config.dest_rect.width = screenWidth/2;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = 0;
      render_config.dest_rect.y_offset = screenHeight/2;
    }
  else if(presetScreenConfig == DISPLAY_QUARTER_BOTTOM_RIGHT)
    {
      render_config.dest_rect.width = screenWidth/2;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = screenWidth/2;
      render_config.dest_rect.y_offset = screenHeight/2;
    }
  //SIXTH
  else if(presetScreenConfig == DISPLAY_SIXTH_TOP_LEFT)
    {
      render_config.dest_rect.width = screenWidth/3;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = 0;
      render_config.dest_rect.y_offset = 0;
    }
  else if(presetScreenConfig == DISPLAY_SIXTH_TOP_MIDDLE)
    {
      render_config.dest_rect.width = screenWidth/3;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = screenWidth/3;
      render_config.dest_rect.y_offset = 0;
    }
  else if(presetScreenConfig == DISPLAY_SIXTH_TOP_RIGHT)
    {
      render_config.dest_rect.width = screenWidth/3;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = (screenWidth/3)*2;
      render_config.dest_rect.y_offset = 0;
    }
  else if(presetScreenConfig == DISPLAY_SIXTH_BOTTOM_LEFT)
    {
      render_config.dest_rect.width = screenWidth/3;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = 0;
      render_config.dest_rect.y_offset = screenHeight/2;
    }
  else if(presetScreenConfig == DISPLAY_SIXTH_BOTTOM_MIDDLE)
    {
      render_config.dest_rect.width = screenWidth/3;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = screenWidth/3;
      render_config.dest_rect.y_offset = screenHeight/2;
    }
  else if(presetScreenConfig == DISPLAY_SIXTH_BOTTOM_RIGHT)
    {
      render_config.dest_rect.width = screenWidth/3;
      render_config.dest_rect.height = screenHeight/2;
      render_config.dest_rect.x_offset = (screenWidth/3)*2;
      render_config.dest_rect.y_offset = screenHeight/2;
    }
  
  render_config.fullscreen = OMX_FALSE;  
  render_config.noaspect = OMX_FALSE;
  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));
}


//sets the image format of the image_encode component cant be done while executing
// WEIRDLY ONLY ACCEPTS JPEG?
// THE CAMERA REFUSES TO OUTPUT ANYTHING BUT YUV
// THIS COMPNENT WILL ONLY SAVE TO JPEG WHEN YUV IS THE INPUT :(

void setParamImageFormat(COMPONENT_T *image_encode, enum formatType formatType)
{
  printf("in setParamImageFormat\n");
  OMX_ERRORTYPE OMXstatus;
  
  //image format stucture */
  OMX_IMAGE_PARAM_PORTFORMATTYPE image_format;  
  memset(&image_format, 0, sizeof(image_format));
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);

  image_format.nPortIndex = 341;

  if(formatType == JPEG_HIGH_FORMAT || formatType == JPEG_MEDIUM_FORMAT || formatType == JPEG_LOW_FORMAT)
    {
      image_format.eCompressionFormat = OMX_IMAGE_CodingJPEG;
        
      OMX_IMAGE_PARAM_QFACTORTYPE compression_format;
      memset(&compression_format, 0, sizeof(compression_format));
      compression_format.nVersion.nVersion = OMX_VERSION;
      compression_format.nSize = sizeof(compression_format);

      compression_format.nPortIndex = 341;
      if(formatType == JPEG_HIGH_FORMAT)
	compression_format.nQFactor = 100;
      else if (formatType == JPEG_MEDIUM_FORMAT)
	compression_format.nQFactor = 60;
      else if (formatType == JPEG_LOW_FORMAT)
	compression_format.nQFactor = 10;
      
      OMXstatus = OMX_SetParameter(ilclient_get_handle(image_encode), OMX_IndexParamQFactor, &compression_format);
      if(OMXstatus != OMX_ErrorNone)
	printf("Error Setting Paramter Error = %s\n", err2str(OMXstatus));
    }
  
  OMXstatus = OMX_SetParameter(ilclient_get_handle(image_encode), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Paramter Error = %s\n", err2str(OMXstatus));
}


//currently needs the image encode to be executing and a tunnel inplace
//this maybe should be ensured through the init 
void savePhoto(COMPONENT_T *camera, COMPONENT_T *image_encode, FILE *file_out)
{
  printf("in savePhoto\n");
  
  OMX_ERRORTYPE OMXstatus;
  OMX_BUFFERHEADERTYPE *decode_out;
  
  printf("capture started\n");

  // needed to notify camera component of image capture
  OMX_CONFIG_PORTBOOLEANTYPE still_capture_in_progress;
  memset(&still_capture_in_progress, 0, sizeof(still_capture_in_progress));
  still_capture_in_progress.nVersion.nVersion = OMX_VERSION;
  still_capture_in_progress.nSize = sizeof(still_capture_in_progress);
  still_capture_in_progress.nPortIndex = 72;
  still_capture_in_progress.bEnabled = OMX_FALSE;

  //tell API port is taking picture - appears to be nessesery!
  still_capture_in_progress.bEnabled = OMX_TRUE;
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera),
			       OMX_IndexConfigPortCapturing,
			       &still_capture_in_progress);  
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to set Config (1)\n");
      exit(EXIT_FAILURE);
    }  

  while(1)
    {
      decode_out = ilclient_get_output_buffer(image_encode, 341, 1/*blocking*/);
      printf("decode_out bytes = %d   :   ", decode_out->nFilledLen);
      printf("decode_out bufferflags = %d\n", decode_out->nFlags);
      
      if(decode_out->nFilledLen != 0) 
	{
	fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out);
        
	}
      if(decode_out->nFlags == 1)
	{
	  OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
	  break;
	}
      OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
    }

  //tell API port is finished capture
  memset(&still_capture_in_progress, 0, sizeof(still_capture_in_progress));
  still_capture_in_progress.nVersion.nVersion = OMX_VERSION;
  still_capture_in_progress.nSize = sizeof(still_capture_in_progress);
  still_capture_in_progress.nPortIndex = 72;
  still_capture_in_progress.bEnabled = OMX_FALSE;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera),
			       OMX_IndexConfigPortCapturing,
			       &still_capture_in_progress);  
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to set Config (1)\n");
      exit(EXIT_FAILURE);
    }  
  
  printf("captureSaved\n");
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
