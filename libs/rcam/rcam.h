// place holder for rcam header files

#ifndef _RCAM_H
#define _RCAM_H

#include "bcm_host.h"
#include "ilclient.h"

/////////////////////////////////////////////////////////////////
// Data Stuctures

enum rcam_command
{
    NO_COMMAND = 0,
    SET_PREVIEW_RES = 10,
    SET_PREVIEW_FRAMERATE = 11,
    START_PREVIEW = 20,
    STOP_PREVIEW = 21,
    TAKE_PHOTO = 30
};

//needed as a thread can only be passed 1 argument
struct cameraControl
{
  ILCLIENT_T *client;
  pthread_mutex_t mutexPtr = PTHREAD_MUTEX_INITALIZER;
  //preview
  bool previewRunning;
  bool previewchanged;
  int previewWidth;
  int previewHeight;
  //photo
  bool takePhoto;
  bool photoChanged;
  int photoHeight;
  int photoWidth;
  //display (renderer)
  bool displayChanged;
  int displayType;
  /*
  future implementations may change things like brightness etc
   */
}

/////////////////////////////////////////////////////////////////
// Function Prototypes

int testFunction(ILCLIENT_T *client);

int InitServerRcam(void *VoidPtrArgs);

#endif // _RCAM_H
