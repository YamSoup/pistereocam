  /*
    use ilclient_GetParameter, ilclient_SetParameter to setup components before executing state
    use ilclient_GetConfig and ilclient_SetConfig to change settings while in executing state
    
    some settings can be changed before executing and some while executing

    all 4 functions use OMX_INDEXTYPE enumeration to specify what settings are being changed

    for index for each component look at
    http://www.jvcref.com/files/PI/documentation/ilcomponents/index.html

    for relevent data stuctures see
    http://maemo.org/api_refs/5.0/beta/libomxil-bellagio/_o_m_x___index_8h.html 

    POSSIBLY USE WAIT() to avoid a busy wait.
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "rcam.h"

/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data); 

/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{

  ILCLIENT_T *client;
  OMX_ERRORTYPE OMXstatus;
  
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

  struct screenSizeStruct screenSize;
  screenSize = returnScreenSize();

  
  struct cameraControl cameraControl;
  //possibly make a function in rcam to give default values
  cameraControl.client = client;
  pthread_mutex_init(&cameraControl.mutexPtr, NULL);
  cameraControl.rcamDeInit = false;

  cameraControl.previewChanged = false;
  cameraControl.previewWidth = 320;
  cameraControl.previewHeight = 240;
  cameraControl.previewFramerate = 30;

  cameraControl.takePhoto = false;
  cameraControl.photoChanged = false;
  cameraControl.photoWidth = 2591; //max settings
  cameraControl.photoHeight = 1944; //max settings

  cameraControl.displayChanged = false;
  cameraControl.displayType = 1; //change to enum

  pthread_t threadid;
  int rc;
  rc = pthread_create(&threadid, NULL, initLocalCamera, (void *)&cameraControl);
  if(rc)
    {
      printf("ERROR creating rcam thread, error = %d\n", rc);
      exit(EXIT_FAILURE);
    }

  sleep(2);
  //////////////////////////////////////////////////////
  //       !!!!!!!!!!!!
  //need functions to change the structure like remote.c
  printf("in main\n");
  printf("screen width = %d\nscreen height = %d\n", screenSize.width, screenSize.height);
  //  changePreviewRes(&cameraControl, screenSize.width/2, (int)((float)(screenSize.width/2)*0.75), 30);
  //sleep(1);
  changePreviewRes(&cameraControl, screenSize.width, screenSize.height, 20);
  
  

  takePhoto(&cameraControl);
  sleep(1);
  changeCaptureRes(&cameraControl, 800, 600);
  sleep(2);  
  takePhoto(&cameraControl);
  sleep (3);
  
  deInit(&cameraControl);
  
  // wait for the thread to complete
  pthread_join(threadid, NULL);

  // OMX drinit
  OMX_Deinit();
  // destroy client
  ilclient_destroy(client);

  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error!!! %s\n", err2str(data));
  //exit(EXIT_FAILURE);
}
