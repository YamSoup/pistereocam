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
  int PThreadResult; // for pthread
  
  //client camera variables
  COMPONENT_T *client_video_render = NULL;
  OMX_ERRORTYPE OMXstatus;
  
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

    
  /////////////////////////////////////////////////////////////////
  // Initalize Components
  /////////////////////////////////////////////////////////////////
  
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

  PThreadResult = pthread_create(&RcamThreadid, NULL, initServerRcam, (void *)&cameraControl);
  if(PThreadResult)
    {
      printf("ERROR creating rcam thread, error = %d\n", PThreadResult);
      exit(EXIT_FAILURE);
    }
  
  
  //sleep for 2 secs
  sleep(2);

  //changePreviewRes(&cameraControl, 320, 320, 15);

  sleep(10);

  takePhoto(&cameraControl);

  sleep(2);
  
  deInit(&cameraControl);
  pthread_join(RcamThreadid, NULL);

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
