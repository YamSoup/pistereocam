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

//stuff for the one_button_api
#include <wiringPi.h>
#include <stdint.h>
#include "one_button_api.h"

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
  int result;
  
  //local camera variables
  COMPONENT_T *local_camera = NULL, *local_video_render = NULL;
  TUNNEL_T tunnel_local_cam_to_local_video_render;
  memset(&tunnel_local_cam_to_local_video_render, 0, sizeof(tunnel_local_cam_to_local_video_render));
  /*
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
  */
  //client camera variables
  COMPONENT_T *client_video_render = NULL;

  OMX_ERRORTYPE OMXstatus;

  uint32_t screen_width = 0, screen_height = 0;

  OMX_BUFFERHEADERTYPE *client_video_render_in;

  int numbytes;
  char char_buffer[12];
  /*
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
  */

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

  struct cameraControl localCameraControl;
  localCameraControl.client = client;
  pthread_mutex_init(&localCameraControl.mutexPtr, NULL);
  localCameraControl.rcamDeInit = false;

  localCameraControl.takePhoto = false;
  
  localCameraControl.previewChanged = false;
  localCameraControl.previewWidth = 320;
  localCameraControl.previewHeight = 240;
  localCameraControl.previewFramerate = 15;
  
  localCameraControl.photoChanged = false;
  localCameraControl.photoWidth = 2591;
  localCameraControl.photoHeight = 1944;

  localCameraControl.displayChanged = false;
  localCameraControl.displayType = DISPLAY_SIDEBYSIDE_LEFT;

  pthread_t localCamThreadID;


  result = pthread_create(&localCamThreadID, NULL, initLocalCamera, (void *)&localCameraControl);
  if(result)
    {
      printf("ERROR creating local camera thread, error = %d\n");
      exit(EXIT_FAILURE);
    }
  
  /////////////////////////////////////////////////////////////////
  // rcam :)
  /////////////////////////////////////////////////////////////////

  struct cameraControl cameraControl;
  //possibly make a function in rcam to give default values
  cameraControl.client = client;
  pthread_mutex_init(&cameraControl.mutexPtr, NULL);
  cameraControl.rcamDeInit = false;

  cameraControl.takePhoto = false;

  cameraControl.previewChanged = false;
  cameraControl.previewWidth = 320;
  cameraControl.previewHeight = 240;
  cameraControl.previewFramerate = 15;

  cameraControl.photoChanged = false;
  cameraControl.photoWidth = 2591; //max settings
  cameraControl.photoHeight = 1944; //max settings

  cameraControl.displayChanged = false;
  cameraControl.displayType = DISPLAY_SIDEBYSIDE_RIGHT;

  pthread_t RcamThreadid;

  result = pthread_create(&RcamThreadid, NULL, initServerRcam, (void *)&cameraControl);
  if(result)
    {
      printf("ERROR creating rcam thread, error = %d\n", result);
      exit(EXIT_FAILURE);
    }

  sleep(2);

  //button stuff
  result = 0;
  pthread_t button_id;
  
  memset(&buttonControl, 0, sizeof buttonControl);
  pthread_mutex_init(&buttonControl.mutexPtr, NULL);
  
  result = wiringPiSetup();
  printf("WiringPi result = %d\n", result);  
  
  pinMode(PIN_NUM, INPUT);
  pullUpDnControl(PIN_NUM, PUD_UP);
  
  result = pthread_create(&button_id, NULL, myButtonPoll, (void*)&buttonControl);
  printf("button_id thread result = %d\n", result);
  
  printf("pre main loop\n\n");
  while(1)
    {
      pthread_mutex_lock(&buttonControl.mutexPtr);
      if (buttonControl.takePhoto == true)
	{
	  printf("take photo\n");
	  takePhoto(&cameraControl);
	  takePhoto(&localCameraControl);
	  system("scp pi@192.168.0.22:photo* ~/Desktop");
	  buttonControl.takePhoto = false;
	  usleep(2000);
	}
      if (buttonControl.exitCountReached == true)
	{
	  printf("exiting\n"); break;
	}
      
      
      //do stuff
      pthread_mutex_unlock(&buttonControl.mutexPtr);
      usleep(400);
      
    }
  
    
  deInit(&localCameraControl);
  deInit(&cameraControl);
  pthread_join(localCamThreadID, NULL);
  pthread_join(RcamThreadid, NULL);

  pthread_join(button_id, NULL);
  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //Disable components
  

  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  //sleep to help scp copy last file
  sleep(3);
  
  //pthread_exit(NULL);
  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}
