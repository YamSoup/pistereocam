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
//stuff for the one_button_api
#include <wiringPi.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>


//pree button headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#include "bcm_host.h"
#include "ilclient.h"
#include "one_button_api.h" //apart from this one

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
  //sleep(2);
  //changePreviewRes(&cameraControl, screenSize.width, screenSize.height, 20);



  //button stuff
  int result = 0;
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
	  //usleep(2000);
	  buttonControl.takePhoto = false;
	}
      if (buttonControl.exitCountReached == true)
	{
	  printf("exiting\n"); break;
	}
      
      
      //do stuff
      pthread_mutex_unlock(&buttonControl.mutexPtr);
      usleep(400);
      
    }
    
  deInit(&cameraControl);
  
  // wait for the thread to complete
  printf("join button_id thread\n");
  pthread_join(button_id, NULL);
  printf("join thread_id thread\n");
  pthread_join(threadid, NULL);

  // OMX drinit
  OMX_Deinit();
  // destroy client
  ilclient_destroy(client);

  printf("\nexit success\n");
  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error!!! %s\n", err2str(data));
  //exit(EXIT_FAILURE);
}
