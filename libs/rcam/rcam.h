// place holder for rcam header files

#ifndef _RCAM_H
#define _RCAM_H

#include "bcm_host.h"
#include "ilclient.h"

#include <stdio.h>
#include <stdbool.h>

#include <pthread.h>

/////////////////////////////////////////////////////////////////
// Data Stuctures

//used to comunicate with remoteCam
enum rcam_command
{
    NO_COMMAND = 0,
    SET_PREVIEW_RES = 10,
    SET_PREVIEW_FRAMERATE = 11,
    START_PREVIEW = 20,
    STOP_PREVIEW = 21,
    TAKE_PHOTO = 30,
    END_REMOTE_CAM = 99
};

//used in setRenderConfig
enum displayTypes
  {
    DISPLAY_FULLSCREEN = 1,
    DISPLAY_SIDEBYSIDE_LEFT = 21,
    DISPLAY_SIDEBYSIDE_RIGHT = 22,
    DISPLAY_QUARTER_TOP_LEFT = 41,
    DISPLAY_QUARTER_TOP_RIGHT = 42,
    DISPLAY_QUARTER_BOTTOM_LEFT = 43,
    DISPLAY_QUARTER_BOTTOM_RIGHT = 44,
    DISPLAY_SIXTH_TOP_LEFT = 61,
    DISPLAY_SIXTH_TOP_MIDDLE = 62,
    DISPLAY_SIXTH_TOP_RIGHT = 63,
    DISPLAY_SIXTH_BOTTOM_LEFT = 64,
    DISPLAY_SIXTH_BOTTOM_MIDDLE = 65,
    DISPLAY_SIXTH_BOTTOM_RIGHT = 66,    
  };

//used in setParamImageFormat
enum formatType
{
  JPEG_HIGH_FORMAT = 1,
  JPEG_MEDIUM_FORMAT = 2,
  JPEG_LOW_FORMAT = 3
};

//used for returnScreenSize
struct screenSizeStruct
{
  uint32_t width;
  uint32_t height;
};

//needed as a thread can only be passed 1 argument
struct cameraControl
{
  ILCLIENT_T *client;
  pthread_mutex_t mutexPtr;
  bool rcamDeInit;
  bool takePhoto;
  //preview
  bool previewChanged;
  bool previewDisplayed;
  int previewWidth;
  int previewHeight;
  //photo
  bool photoChanged;
  int photoHeight;
  int photoWidth;
  //display (renderer)
  bool displayChanged;
  enum displayTypes displayType;
  //display size has been removed replaced with a function
  /*
  future implementations may change things like brightness etc
   */
};

/////////////////////////////////////////////////////////////////
// Function Prototypes

int testFunction(ILCLIENT_T *client);

void *initLocalCamera(void *VoidPtrArgs);
void *initServerRcam(void *VoidPtrArgs);

// functions that were originally in camera.c
struct screenSizeStruct returnScreenSize(void);

void setCaptureRes(COMPONENT_T *camera, int width, int height);
void setPreviewRes(COMPONENT_T *camera, int width, int height);
void setRenderConfig(COMPONENT_T *video_render, enum displayTypes presetScreenConfig);
void setParamImageFormat(COMPONENT_T *image_encode, enum formatType formatType);

void savePhoto(COMPONENT_T *camera, COMPONENT_T *image_encode, FILE *file_out);

//functions that manipulate the cameraControl struct that ultimatly controls the camera
void deInit(struct cameraControl *toChange);
void takePhoto(struct cameraControl *toChange);
void changePreviewRes(struct cameraControl *toChange, int newWidth, int newHeight);
void changeCaptureRes(struct cameraControl *toChange, int newWidth, int newHeight);
void changeDisplayType(struct cameraControl *toChange, enum displayTypes newDisplayType);



#endif // _RCAM_H
