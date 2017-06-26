#include "rcam.h"

#include "bcm_host.h"
#include "ilclient.h"

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "print_OMX.h"
#include "socket_helper.h"

#define PORT "8039"

/*
working in camera.c still need to implement
This is a test comment to see if git is working on windows
 */

//untested
// the idea is that the function takes in the filePrefix and a Helper number (the number we belive at that time is where the files are at)
// and then the function checks if the filename (comination of filePrefix and  is available if not it increments until a filename is availble and then returns a long int of the current count
// to be used as the next HelperNumber, if effiency is not needed it can be called with 0 for the helper number and the return value ignored
// might replace this or add another fuction that spits out a name.

unsigned int fileFindNext(char* filePrefix,unsigned int helperNumber)
{
  char currentFileName[80];
  unsigned int count = helperNumber;
  char countString[6];
  bool loop = true;
  while(loop)
    {
      sprintf(countString, "_%05d\n", count);
      strcpy(currentFileName, filePrefix);
      strcat(currentFileName, countString);
      if( access(currentFileName, F_OK) != -1)
	{
	  if(count == 65535)
	    {
	      fprintf(stderr, "Error! end of useable numbers for filename");
	      exit(EXIT_FAILURE);
	    }
	  count++; continue;
	}
      else
	{
	return count;
	}
    }
}

void *initLocalCamera(void *VoidPtrArgs)
{
  struct cameraControl *currentArgs = VoidPtrArgs;

  pthread_mutex_lock(&currentArgs->mutexPtr);
  ILCLIENT_T *client = currentArgs->client;
  pthread_mutex_unlock(&currentArgs->mutexPtr);

  //////////////////
  //VARIABLES

  COMPONENT_T *camera = NULL, *video_render = NULL, *image_encode = NULL;
  OMX_ERRORTYPE OMXstatus;

  TUNNEL_T tunnel_camera_to_render, tunnel_camera_to_encode;
  memset(&tunnel_camera_to_render, 0, sizeof(tunnel_camera_to_render));
  memset(&tunnel_camera_to_encode, 0, sizeof(tunnel_camera_to_encode));

  //////////////////
  // STARTUP

  //////////////////
  // Initalize Components

  //////////////////
  ////initialise camera

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
  setPreviewRes(camera, currentArgs->previewWidth, currentArgs->previewHeight, currentArgs->previewFramerate);

  //////////////////
  ////Initialise video render

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

  ////////////////////
  ////Initalise Image Encoder

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

  //image format Param set
  setParamImageFormat(image_encode, JPEG_HIGH_FORMAT);

  ////////////////////
  // enable components and tunnels

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

  /////////////////
  // Main initLocalCamera() Loop

  while(1)
    {
      pthread_mutex_lock(&currentArgs->mutexPtr);
      if (currentArgs->previewChanged == true)
	{
	  ilclient_disable_tunnel(&tunnel_camera_to_render);
	  OMXstatus = ilclient_change_component_state(camera, OMX_StatePause);
	  setPreviewRes(camera,
			currentArgs->previewWidth,
			currentArgs->previewHeight,
			currentArgs->previewFramerate);
	  OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
  	  ilclient_enable_tunnel(&tunnel_camera_to_render);
	  currentArgs->previewChanged = false;
	}
      else if (currentArgs->photoChanged == true)
	{
	  ilclient_disable_tunnel(&tunnel_camera_to_encode);
	  setCaptureRes(camera, currentArgs->photoWidth, currentArgs->photoHeight);
  	  ilclient_enable_tunnel(&tunnel_camera_to_encode);
	  currentArgs->photoChanged = false;
	}
      else if (currentArgs->displayChanged == true)
	{
	  setRenderConfig(video_render, currentArgs->displayType);
	  currentArgs->displayChanged = false;
	}
      else if (currentArgs->takePhoto == true)
	{
	  savePhoto(camera, image_encode, "local_photo");
	  //TODO close file and open next!
	  currentArgs->takePhoto = false;
	}
      //loop termination
      else if (currentArgs->rcamDeInit == true)
	{
	  printf("end\n");
	  pthread_mutex_unlock(&currentArgs->mutexPtr);
	  break;
	}

      pthread_mutex_unlock(&currentArgs->mutexPtr);
      //try below to avoid busy wait
      //usleep(500);
    }

  ///////////////
  //CLEANUP

  
  //Disable components

  //call pthread_exit so caller can join
  pthread_exit(NULL);

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

  OMX_PARAM_PORTDEFINITIONTYPE render_params;
  memset(&render_params, 0, sizeof(render_params));
  render_params.nVersion.nVersion = OMX_VERSION;
  render_params.nSize = sizeof(render_params);
  render_params.nPortIndex = 90;

  enum rcam_command rcam_command = NO_COMMAND;

  /////////////////////////////////////////////////////////////////
  // SOCKET STUFF

  printf("start of socket stuff in rcam\n");

  int socket_fd = 0, client_socket_fd = 0;

  socket_fd = getAndBindTCPServerSocket(PORT);
  printf("waiting for remotecam to connect\n");
  client_socket_fd = listenAndAcceptTCPServerSocket(socket_fd, 10/*backlog*/);

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
  // !!!
  // needs the checks that the local camera does
  // to ensure sanity (OR DOES IT ALREADY DO THIS ? IT MIGHT BE)

  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  render_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  render_params.format.video.nFrameWidth = currentArgs->previewWidth;
  render_params.format.video.nFrameHeight = currentArgs->previewHeight;
  render_params.format.video.nStride = currentArgs->previewWidth;
  render_params.format.video.nSliceHeight = currentArgs->previewHeight;
  render_params.format.video.xFramerate = currentArgs->previewFramerate << 16;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(client_video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting video render port parameters (1)");

  //check(print) the port params
  memset(&render_params, 0, sizeof(render_params));
  render_params.nVersion.nVersion = OMX_VERSION;
  render_params.nSize = sizeof(render_params);
  render_params.nPortIndex = 90;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  print_OMX_PARAM_PORTDEFINITIONTYPE(render_params);


  //ask ilclient to allocate buffers for client_video_render

  printf("enable client_video_render_input port\n");
  ilclient_enable_port_buffers(client_video_render, 90, NULL, NULL,  NULL);
  ilclient_enable_port(client_video_render, 90);

  //change preview render to executing
  OMXstatus = ilclient_change_component_state(client_video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printf("client_video_render state is ");
  printState(ilclient_get_handle(client_video_render));
  printf("***\n");

  //set the position on the screen
  setRenderConfig(client_video_render, currentArgs->displayType);

  ////////////////////////////////////////////////////////////
  // INITIATE RCAM_REMOTE_SLAVE

  char char_buffer[12];

  //handshake
  printf("waiting to recive handshake ... \n");
  read(client_socket_fd, char_buffer, 11);
  printf("handshake result = %s", char_buffer);

  write(client_socket_fd, "got\0", sizeof(char)*4);
  //initalize preview
  write(client_socket_fd, &currentArgs->previewWidth, sizeof(currentArgs->previewWidth));
  write(client_socket_fd, &currentArgs->previewHeight, sizeof(currentArgs->previewHeight));
  write(client_socket_fd, &currentArgs->previewFramerate, sizeof(currentArgs->previewFramerate));
  //initalize capture
  write(client_socket_fd, &currentArgs->photoWidth, sizeof(currentArgs->photoWidth));
  write(client_socket_fd, &currentArgs->photoHeight, sizeof(currentArgs->photoHeight));



  pthread_mutex_unlock(&currentArgs->mutexPtr);

  ////////////////////////////////////////////////////////////
  //// Main Thread Loop

  
  void * preview_buffer;
  preview_buffer = malloc(render_params.nBufferSize + 1 );
  printf("***preview nBufferSize = %d\n", render_params.nBufferSize);

  int photo_buffer_size;
  void * photo_buffer;
  //photo_buffer = malloc();
  
  long int num_bytes = 0;

  enum rcam_command current_command = START_PREVIEW;
  printf("current_command = %d\n", current_command);
  printf("sending command ...");
  write(client_socket_fd, &current_command, sizeof(current_command));
  printf("sent command\n");

  current_command = NO_COMMAND;
	  // possibly abandon current command struct
	  // and serialize the cameraControlStruct and sent that instead
	  // see: http://stackoverflow.com/questions/1577161/passing-a-structure-through-sockets-in-c


  int count = 500;
  //modify to inifinate loop when control functions are writen
  while(1)
    {
      pthread_mutex_lock(&currentArgs->mutexPtr);

      if (currentArgs->previewChanged == true)
	{
	  printf("in previewChanged !\n");
	  //needs to:
	  //change the renderer params this side
	  OMXstatus = ilclient_change_component_state(client_video_render, OMX_StateIdle);

	  memset(&render_params, 0, sizeof(render_params));
	  render_params.nVersion.nVersion = OMX_VERSION;
	  render_params.nSize = sizeof(render_params);
	  render_params.nPortIndex = 90;

	  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render),
				    OMX_IndexParamPortDefinition,
				    &render_params);
	  if (OMXstatus != OMX_ErrorNone)
	    printf("Error Getting video render port parameters (in loop)");

	  render_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	  render_params.format.video.nFrameWidth = currentArgs->previewWidth;
	  render_params.format.video.nFrameHeight = currentArgs->previewHeight;
	  render_params.format.video.nStride = currentArgs->previewWidth;
	  render_params.format.video.nSliceHeight = currentArgs->previewHeight;
	  render_params.format.video.xFramerate = currentArgs->previewFramerate << 16;

	  OMXstatus = OMX_SetConfig(ilclient_get_handle(client_video_render),
				    OMX_IndexParamPortDefinition,
				    &render_params);
	  if (OMXstatus != OMX_ErrorNone)
	    printf("Error Setting video render port parameters (in loop)");

	  OMXstatus = ilclient_change_component_state(client_video_render, OMX_StateExecuting);

	  //resize the preview buffer

	  memset(&render_params, 0, sizeof(render_params));
	  render_params.nVersion.nVersion = OMX_VERSION;
	  render_params.nSize = sizeof(render_params);
	  render_params.nPortIndex = 90;

	  OMXstatus = OMX_GetConfig(ilclient_get_handle(client_video_render),
				    OMX_IndexParamPortDefinition,
				    &render_params);
	  if (OMXstatus != OMX_ErrorNone)
	    printf("Error Getting video render port parameters (in loop)");

	  free(preview_buffer);
	  preview_buffer = malloc(render_params.nBufferSize + 1);

	  //change the preview on the remote side
	  current_command = SET_PREVIEW_RES;
	  write(client_socket_fd, &current_command, sizeof(current_command));
	  //send needed paramaters to rcam_remote_slave for the preview change
	  write(client_socket_fd, &currentArgs->previewWidth, sizeof(currentArgs->previewWidth));
	  write(client_socket_fd, &currentArgs->previewHeight, sizeof(currentArgs->previewHeight));
	  write(client_socket_fd, &currentArgs->previewFramerate, sizeof(currentArgs->previewFramerate));
	  //!!!
	  //possibly wait for confirmation
	  currentArgs->previewChanged = false;
	  sleep(1);
	}
      else if (currentArgs->photoChanged == true)
	{
	  printf("in photoChanged !\n");
	  //needs to:
	  //change the capture res on the remote side
	  current_command = SET_CAPTURE_RES;
	  write(client_socket_fd, &current_command, sizeof(current_command));
	  //send needed paramaters to rcam_remote_slave for the photo change
	  write(client_socket_fd, &currentArgs->previewWidth, sizeof(currentArgs->previewWidth));
	  write(client_socket_fd, &currentArgs->previewHeight, sizeof(currentArgs->previewHeight));
	  //change the photo_buffer??
	  read(client_socket_fd, &photo_buffer_size, sizeof(photo_buffer_size));
	  free(photo_buffer);
	  photo_buffer = malloc(photo_buffer_size + 1);
	  //!!!
	  //possibly wait for confirmation
	  currentArgs->photoChanged = false;
	}
      else if (currentArgs->displayChanged == true)
	{
	  printf("in displayChanged !\n");
	  setRenderConfig(client_video_render, currentArgs->displayType);
	  currentArgs->displayChanged = false;
	}
      else if (currentArgs->takePhoto == true)
	{
	  printf("in takePhoto !\n");
	  //needs to:
	  //send command and then recive the capture
	  current_command = TAKE_PHOTO;
	  write(client_socket_fd, &current_command, sizeof(current_command));	  
	  currentArgs->takePhoto = false;
	  printf("end of take photo\n");
	}
      //loop terminationcurrent_command = TAKE_PHOTO;
      else if(currentArgs->rcamDeInit)
	{
	  printf("in rcamDiInit !\n");
	  current_command = END_REMOTE_CAM;
	  write(client_socket_fd, &current_command, sizeof(current_command));
  	  printf("END_REMOTE_CAM sent\n");
	  break; //exits while loop
	}
      else
	{
	  printf("no commands in struct to parse !\n");
	  //send no command
	  current_command = NO_COMMAND;
	  write(client_socket_fd, &current_command, sizeof(current_command));

	  printf("get a buffer to process\n");
	  printf("waiting to recv buffer of size %d... ", render_params.nBufferSize);
	  num_bytes = read(client_socket_fd,
			   preview_buffer,
			   render_params.nBufferSize);
	  while (num_bytes < render_params.nBufferSize)
	    {
	      num_bytes += read(client_socket_fd,
				preview_buffer + num_bytes,
				render_params.nBufferSize - num_bytes);
	    }
	  printf("buffer recived, recived %ld bytes\n", num_bytes);

	  //change nAllocLen in bufferheader
	  client_video_render_in = ilclient_get_input_buffer(client_video_render, 90, 1);
	  memcpy(client_video_render_in->pBuffer, preview_buffer, render_params.nBufferSize);
	  printf("copied buffer form preview_buffer into client_video_render_in\n");
	  //fix alloc len
	  client_video_render_in->nFilledLen = render_params.nBufferSize;

	  //empty buffer into render component
	  OMX_EmptyThisBuffer(ilclient_get_handle(client_video_render), client_video_render_in);
	  count++;
	  printf("Emptied buffer --- count = %d\n", count);
	}
      pthread_mutex_unlock(&currentArgs->mutexPtr);
      usleep(500);
    }

  ////////////////////////////////////////////////////////////
  //// end of thread Cleanup

  //free buffer memory
  free(preview_buffer);
  printf("preview_buffer memory free\n");
  //!free ilobjects and make sure all allocated memory is free!

  //!free sockets try to ensure no zombies
  printf("exiting rcam thread");
  pthread_exit(NULL);
}

/////////////////////////////////////////////////////////
// Functions that were originally in camera.c
/////////////////////////////////////////////////////////

//function returns screensize seperated only to make porting easier
//NOTE: super special function only works after bcm_host_init() and possibly others
struct screenSizeStruct returnScreenSize(void)
{
  //currently very broken
  struct screenSizeStruct currentScreenSize;
  uint32_t currentScreenWidth = 0;
  uint32_t currentScreenHeight = 0;

  //super special broadcom only function
  graphics_get_display_size(0/*framebuffer 0*/, &currentScreenWidth, &currentScreenHeight);
  currentScreenSize.width = (int)currentScreenWidth;
  currentScreenSize.height = (int)currentScreenHeight;

  return currentScreenSize;
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

  
  //print current config
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 72;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter (2) In setCaptureRes. Error = %s\n", err2str(OMXstatus));

  print_OMX_PARAM_PORTDEFINITIONTYPE(port_params);
  
}


//sets the preview res of the camera
void setPreviewRes(COMPONENT_T *camera, int width, int height, int framerate)
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
  port_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  port_params.format.video.nFrameWidth = width;
  port_params.format.video.nFrameHeight = height;
  port_params.format.video.nStride = width;
  port_params.format.video.nSliceHeight = height;
  port_params.format.video.xFramerate = framerate << 16;
  //set changes
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));


  //print current config
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 70;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter (2) In setPreviewRes. Error = %s\n", err2str(OMXstatus));

  print_OMX_PARAM_PORTDEFINITIONTYPE(port_params);

}

// sets the preview size and position
// takes the enum displayTypes the contents of which should hopefully be self explanitory
void setRenderConfig(COMPONENT_T *video_render, enum displayTypes presetScreenConfig)
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


//in development
void savePhoto(COMPONENT_T *camera, COMPONENT_T *image_encode, char *fileprefix)
{
  printf("in savePhoto\n");

  OMX_ERRORTYPE OMXstatus;
  OMX_BUFFERHEADERTYPE *decode_out;

  //needs to be changed to make use of fileprefix
  // and to increment the file
  FILE *file_out;
  file_out = fopen("local_pic", "wb");

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
	  fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out);
	  OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
	  break;
	}
      //crashes if starts with 0 buffer?
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

  fclose(file_out);
  printf("captureSaved\n");
}

///////////////////////////////////////////////////////////////////////////////////////
//functions that manipulate the cameraControl struct that ultimatly controls the camera
///////////////////////////////////////////////////////////////////////////////////////

//stops the camera loop NEEDS A COROSPONDING pthread_join()
//might be worth trying to include the thread id in the cameraControlStruct
void deInit(struct cameraControl *toChange)
{
  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->rcamDeInit = true;
  pthread_mutex_unlock(&toChange->mutexPtr);
  //pthread_join(toChange->threadid, NULL); THIS WILL ONLY WORK IF THREADID IS INCLUDED IN STRUCT
}
void takePhoto(struct cameraControl *toChange)
{
  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->takePhoto = true;
  pthread_mutex_unlock(&toChange->mutexPtr);
}
void changePreviewRes(struct cameraControl *toChange, int newWidth, int newHeight, int newFramerate)
{
  // these checks will do for now
  // resolutions need to be multiples of 32 (confirmed after much testing)
  int remainder = 0;
  remainder = newWidth % 16;
  if (remainder != 0)
    newWidth = newWidth - remainder;
  remainder = newHeight % 16;
  if (remainder != 0)
    newHeight = newHeight - remainder;

  // check if to large for buffer
  // buffer limit appears to be 2995200 doesn't appear to be effected by framerate?
  // bellow calc does not
  if ((long)newWidth * (long)newHeight > 2000000 )
    {
      printf("resolution to large for buffer");
      return;
    }

  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->previewChanged = true;
  toChange->previewWidth = newWidth;
  toChange->previewHeight = newHeight;
  toChange->previewFramerate = newFramerate;
  pthread_mutex_unlock(&toChange->mutexPtr);
}
void changeCaptureRes(struct cameraControl *toChange, int newWidth, int newHeight)
{
  //modify to check for allowed resolutions
  //and then select the closet sane option
  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->photoChanged = true;
  toChange->photoWidth = newWidth;
  toChange->photoHeight = newHeight;
  pthread_mutex_unlock(&toChange->mutexPtr);
}
void changeDisplayType(struct cameraControl *toChange, enum displayTypes newDisplayType)
{
  pthread_mutex_lock(&toChange->mutexPtr);
  toChange->displayChanged = true;
  toChange->displayType = newDisplayType;
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
