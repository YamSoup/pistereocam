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

/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data); 

/////////////////////////////////////////////////////////////////
//
// FUNCTIONS TO PUT IN RCAM!
//
/////////////////////////////////////////////////////////////////


void setCaptureRes(COMPONENT_T *camera, int width, int height)
{
  //needs to check width and height to see if compatible with rpi

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
  

}

//set preview res
void setPreviewRes(COMPONENT_T *camera, int width, int height)
{
    //needs to check width and height to see if compatible with rpi

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
    port_params.format.video.xFramerate = 24 << 16;
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
    
}



#define FULLSCREEN 1

/*
  this functions sets where the render is displayed on the screen
  presetScreenConfig will be a ENUM in rcam.h 
  presets will include FULLSCREEN, SIDEBYSIDELEFT, SIDEBYSIDERIGHT

  TODO change presetScreenConfig to enum when enum is created
*/
void setRenderConfig(COMPONENT_T *video_render, int presetScreenConfig, int screenWidth, int screenHeight)
{
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
  render_config.fullscreen = OMX_FALSE;
  render_config.noaspect = OMX_FALSE;
 
  render_config.dest_rect.width = screenWidth;
  render_config.dest_rect.height = screenHeight;

  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));  
}

#define JPEG_FORMAT 1
#define BMP_FORMAT 3
/*
Change formatType to enum as above 

CURENTLY THIS IS NOT WORKING ON EXECUTING COMPONENTS
AS PARAM CHANGES CAN ONLY BE DONE BEFORE A COMPONENT IS EXECUTING
TO MOVE COMPONENTS TO IDLE THE TUNNELS WOULD NEED TO BE DISABLED
*/

// horribly broken DONT USE THE SAME STRUCTURE TO GET AND SET!!!!
// WEIRDLY ONLY ACCEPTS JPEG?
// I BELIVE THIS TO BE CAUSED BY THE eColorFormat but I think it needs to match on camera and image_encode

void setParamImageFormat(COMPONENT_T *image_encode, COMPONENT_T *camera, int formatType)
{
  OMX_ERRORTYPE OMXstatus;
  
  //image format stucture */
  OMX_IMAGE_PARAM_PORTFORMATTYPE image_format;  
  memset(&image_format, 0, sizeof(image_format));
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);

  //populate image_format with information from the camera port
  image_format.nPortIndex = 72;

  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Paramter. Error = %s\n", err2str(OMXstatus));
  
  image_format.nPortIndex = 341;
  // image_format.eColorFormat = OMX_COLOR_Format24bitRGB888;
  image_format.eCompressionFormat = OMX_IMAGE_CodingJPEG;
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);

  //print_OMX_IMAGE_PORTDEFINITIONTYPE(image_format);
  OMXstatus = OMX_SetParameter(ilclient_get_handle(image_encode), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error setting Paramter. Error = %s\n", err2str(OMXstatus));
}

//currently needs the image encode to be executing and a tunnel inplace
//this maybe should be ensured through the init 
void savePhoto(COMPONENT_T *camera, COMPONENT_T *image_encode, FILE *file_out)
{
  OMX_ERRORTYPE OMXstatus;
  OMX_BUFFERHEADERTYPE *decode_out;
  
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
      printf("decode_out bytes = %d\n", decode_out->nFilledLen);
      printf("decode_out bufferflags = %d\n\n", decode_out->nFlags);
      
      if(decode_out->nFilledLen != 0) 
	fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out);
      if(decode_out->nFlags == 1)
	{
	  OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
	  break;
	}
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
  
  printf("captureSaved\n");
}


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

  setRenderConfig(camera, FULLSCREEN, screen_width, screen_height);  

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
  setParamImageFormat(image_encode, camera, JPEG_FORMAT);
   
  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////
  
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
  //sleep for 2 secs
  sleep(2);

  printState(ilclient_get_handle(image_encode));
  
  savePhoto(camera, image_encode, file_out2);

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
}
