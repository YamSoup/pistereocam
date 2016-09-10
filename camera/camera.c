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
//
// FUNCTIONS TO PUT IN RCAM!
//
/////////////////////////////////////////////////////////////////



/*
Change formatType to enum as above 

CURENTLY THIS IS NOT WORKING ON EXECUTING COMPONENTS
AS PARAM CHANGES CAN ONLY BE DONE BEFORE A COMPONENT IS EXECUTING
TO MOVE COMPONENTS TO IDLE THE TUNNELS WOULD NEED TO BE DISABLED
*/

// 
// WEIRDLY ONLY ACCEPTS JPEG?
// THE CAMERA REFUSES TO OUTPUT ANYTHING BUT YUV
// THIS COMPNENT WILL ONLY SAVE TO JPEG WHEN YUV IS THE INPUT :(





/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  /////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////

  ILCLIENT_T *client;

  COMPONENT_T *camera = NULL, *video_render = NULL, *image_encode = NULL;
  OMX_ERRORTYPE OMXstatus;
  uint32_t screen_width = 0, screen_height = 0;

  FILE *file_out1, *file_out2;
  file_out1 = fopen("pic1", "wb");
  file_out2 = fopen("pic2", "wb");
  
  TUNNEL_T tunnel_camera_to_render, tunnel_camera_to_encode;
  memset(&tunnel_camera_to_render, 0, sizeof(tunnel_camera_to_render));
  memset(&tunnel_camera_to_encode, 0, sizeof(tunnel_camera_to_encode));
  
    
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
  setCaptureRes(camera, 2592, 1944);

  //change the preview resolution
  setPreviewRes(camera, 320, 240);
  
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

  setRenderConfig(video_render, DISPLAY_SIDEBYSIDE_LEFT);  

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


  //image format stucture */
  setParamImageFormat(image_encode, JPEG_HIGH_FORMAT);
   
  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////


  //DELETE
  //#define DEBUG
#ifndef DEBUG


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
  printState(ilclient_get_handle(camera));

  //change preview render to executing
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(video_render));
  
  //sleep for 2 secs
  sleep(2);

  //enable port and buffers for output por of image encode
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
  printState(ilclient_get_handle(image_encode));

  //////////////////////////////////////////////////////
  // Code that takes picture
  //////////////////////////////////////////////////////

  savePhoto(camera, image_encode, file_out1);
  sleep(2);
  setRenderConfig(video_render, DISPLAY_SIDEBYSIDE_RIGHT);  
  sleep(2);  
  savePhoto(camera, image_encode, file_out2);
  
  //DELETE 
#endif
    
  sleep(2);
  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //close files
  fclose(file_out1);
  fclose(file_out2);
  
  //Disable components
  

  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error!!! %s\n", err2str(data));
  //exit(EXIT_FAILURE);
}
