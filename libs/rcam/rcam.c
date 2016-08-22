//place holder for remote cam helper funstions

#include "rcam.h"
#include "bcm_host.h"
#include "ilclient.h"

/*
what do I want from this

I want server functions to create the preview renderer for the remote camera INITIALIZE
I want to to be able to start and stop the preview from the server

next: I want to be able to control settings for the remote camera from the server
next: I want to be able to take a photo and have it sent to the server

I want to destroy the initlaization

maybe client side functions to simplify/
 */

//test funstion to check i can raise ilclient objexts from a linked file
int testFunction(ILCLIENT_T *client)
{
  COMPONENT_T *cameraTest;
  int result;

  result = ilclient_create_component(client,
                            &cameraTest,
                            "camera",
                            ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_OUTPUT_BUFFERS);
  printState(ilclient_get_handle(cameraTest));

  return result;
}

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
  struct rcamThreadArgs *currentArgs = VoidPtrArgs;
  printf("cameraControl\npreviewWidth: %d\n", currentArgs->previewWidth);

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

  ///////////////////////////////////////////
  ////Initialise client video render////
  ///////////////////////////////////////////
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

  //set the port params to the same as the remote camera

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
  render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
					   |OMX_DISPLAY_SET_FULLSCREEN
					   |OMX_DISPLAY_SET_NOASPECT
					   |OMX_DISPLAY_SET_MODE);
  render_config.fullscreen = OMX_FALSE;
  render_config.noaspect = OMX_FALSE;

  render_config.dest_rect.width = cameraControl->screen_width/2;
  render_config.dest_rect.height = cameraControl->screen_height;
  render_config.dest_rect.x_offset = cameraControl->screen_width/2;

  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(client_video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));


  //ask ilclient to allocate buffers for client_video_render
  printf("enable client_video_render_input port\n");
  ilclient_enable_port_buffers(client_video_render, 90, NULL, NULL,  NULL);
  ilclient_enable_port(client_video_render, 90);


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




}

// does not need to have void pointer attribute as will run in main thread
// This is a template Function for all Functions that will control rcam like take photo
// this will be achived by changing shared memory to control the loop in initServerRcam
int exampleVariableChanger (cameraControl *toChange)
{
  //does not need to be &mutexPtr as is passed as a pointer
  // this is an atempt to not declare it globally
  pthread_mutex_lock(&toChange->mutexPtr); /*check this would actually work*/
  toChange->variable = value;
  pthread_mutex_unlock(&toChange->mutexPtr);
}
