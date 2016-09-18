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
#include <unistd.h> //used for sleep()

#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>

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

  //local camera variables
  COMPONENT_T *local_camera = NULL, *local_video_render = NULL;
  TUNNEL_T tunnel_local_cam_to_local_video_render;
  memset(&tunnel_local_cam_to_local_video_render, 0, sizeof(tunnel_local_cam_to_local_video_render));

  OMX_CONFIG_DISPLAYREGIONTYPE local_render_config;
  memset(&local_render_config, 0, sizeof(local_render_config));
  local_render_config.nVersion.nVersion = OMX_VERSION;
  local_render_config.nSize = sizeof(local_render_config);
  local_render_config.nPortIndex = 90;

  OMX_PARAM_PORTDEFINITIONTYPE local_port_params;
  memset(&local_port_params, 0, sizeof(local_port_params));
  local_port_params.nVersion.nVersion = OMX_VERSION;
  local_port_params.nSize = sizeof(local_port_params);
  local_port_params.nPortIndex = 72;

  //client camera variables
  COMPONENT_T *client_video_render = NULL;

  OMX_ERRORTYPE OMXstatus;

  uint32_t screen_width = 0, screen_height = 0;

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
  ////Local Camera
  ///////////////////////////////////////////

  ///////////////////////////////////
  //initalize local camera component
  ilclient_create_component(client,
			    &local_camera,
			    "camera",
			    ILCLIENT_DISABLE_ALL_PORTS);

  OMXstatus = ilclient_change_component_state(local_camera, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move local_camera to Idle (1)");
      exit(EXIT_FAILURE);
    }

  //change the preview resolution of local_camera using structure local_port_params
  OMXstatus = OMX_GetParameter(ilclient_get_handle(local_camera),
			       OMX_IndexParamPortDefinition,
			       &local_port_params);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr,
	      "Error Getting paramater for local_port_params (1) Error = %s\n",
	      err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }

  local_port_params.format.video.nFrameWidth = 320;
  local_port_params.format.video.nFrameHeight = 240;
  local_port_params.format.video.nStride = 0;
  local_port_params.format.video.nSliceHeight = 0;
  local_port_params.format.video.nBitrate = 0;
  local_port_params.format.video.xFramerate = 0;

  OMXstatus = OMX_SetParameter(ilclient_get_handle(local_camera),
			       OMX_IndexParamPortDefinition,
			       &local_port_params);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr,
	      "Error Setting parameter for local_port_params (1) Error = %s\n",
	      err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }

  ///////////////////////////////////
  //initalize local_video_render
  ilclient_create_component(client,
			    &local_video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS);

  OMXstatus = ilclient_change_component_state(local_video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr,
	      "Unable to move local_video_render to Idle (1) Error = %s\n",
	      err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }

  //set the position and size of display using local_render_config
  local_render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
						 |OMX_DISPLAY_SET_FULLSCREEN
						 |OMX_DISPLAY_SET_NOASPECT
						 |OMX_DISPLAY_SET_MODE);
  local_render_config.fullscreen = OMX_FALSE;
  local_render_config.noaspect = OMX_FALSE;

  local_render_config.dest_rect.width = screen_width/2;
  local_render_config.dest_rect.height = screen_height;

  local_render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(local_video_render),
			    OMX_IndexConfigDisplayRegion,
			    &local_render_config);

  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr,
	      "Unable to set the local_render display with local_render_config, Error = %s",
	      err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }

  //setup the tunnel for the local camera

  set_tunnel(&tunnel_local_cam_to_local_video_render,
	     local_camera, 70,
	     local_video_render, 90);
  ilclient_setup_tunnel(&tunnel_local_cam_to_local_video_render, 0, 0);

  //change components to execution

  OMXstatus = ilclient_change_component_state(local_camera, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr,
	      "Unable to move local_camera to state executing (1), Error = %s",
	      err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(local_camera));

  OMXstatus = ilclient_change_component_state(local_video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr,
	      "Unable to move local_video_render to state executing (1), Error = %s",
	      err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(local_video_render));

  
  /////////////////////////////////////////////////////////////////
  // rcam :)
  /////////////////////////////////////////////////////////////////

  struct cameraControl cameraControl;
  //possibly make a function in rcam to give default values
  cameraControl.client = client;
  pthread_mutex_init(&cameraControl.mutexPtr, NULL);
  cameraControl.rcamDeInit = false;

  cameraControl.previewDisplayed = true;
  cameraControl.previewChanged = false;
  cameraControl.previewWidth = 320;
  cameraControl.previewHeight = 240;

  cameraControl.takePhoto = false;
  cameraControl.photoChanged = false;
  cameraControl.photoWidth = 2591; //max settings
  cameraControl.photoHeight = 1944; //max settings

  cameraControl.displayChanged = false;
  cameraControl.displayType = 1;

  pthread_t threadid;
  int rc;
  rc = pthread_create(&threadid, NULL, initServerRcam, (void *)&cameraControl);
  if(rc)
    {
      printf("ERROR creating rcam thread, error = %d\n", rc);
      exit(EXIT_FAILURE);
    }
  
  
  //sleep for 2 secs
  sleep(10);
  deInit(&cameraControl);
  pthread_join(threadid, NULL);

  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //Disable components


  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  //pthread_exit(NULL);
  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}
