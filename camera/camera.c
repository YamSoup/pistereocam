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


/*
Idea for camera capture on rcam 
Create a buffer large enough to hold the biggest image possible and an int for the size of the final buffer
Transfer the whole image when it is finished 
This will get over the issue of having to wait for the flag to tell when the process has stopped !
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
  OMX_BUFFERHEADERTYPE *decode_out;

  FILE *file_out1;
  file_out1 = fopen("pic1", "wb");
  
  TUNNEL_T tunnel_camera_to_render, tunnel_camera_to_encode;
  memset(&tunnel_camera_to_render, 0, sizeof(tunnel_camera_to_render));
  memset(&tunnel_camera_to_encode, 0, sizeof(tunnel_camera_to_encode));

  OMX_CONFIG_DISPLAYREGIONTYPE render_config;
  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90; 

  //res
  OMX_PARAM_PORTDEFINITIONTYPE port_params;
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 72;

  //camera still capture stuctures
  OMX_IMAGE_PARAM_PORTFORMATTYPE image_format;
  memset(&image_format, 0, sizeof(image_format));
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);
  
  OMX_CONFIG_PORTBOOLEANTYPE still_capture_in_progress;
  memset(&still_capture_in_progress, 0, sizeof(still_capture_in_progress));
  still_capture_in_progress.nVersion.nVersion = OMX_VERSION;
  still_capture_in_progress.nSize = sizeof(still_capture_in_progress);
  still_capture_in_progress.nPortIndex = 72;
  still_capture_in_progress.bEnabled = 0;
  
    
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
  //prepopulate structure
  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Paramter. Error = %s\n", err2str(OMXstatus));
  //change needed params
  port_params.format.image.nFrameWidth = 2592; //maxsettings
  port_params.format.image.nFrameHeight = 1944;
  port_params.format.image.nStride = 0; //needed! set to 0 to recalculate
  port_params.format.image.nSliceHeight = 0;  //notneeded?
  //set changes
  OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Paramter. Error = %s\n", err2str(OMXstatus));

  //change the preview resolution
  //reuse port params
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 70;
  //prepopulate structure
  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));
  //change needed params
  port_params.format.video.nFrameWidth = 320;
  port_params.format.video.nFrameHeight = 240;
  port_params.format.video.nStride = 0;
  port_params.format.video.nSliceHeight = 0;
  port_params.format.video.nBitrate = 0;
  port_params.format.video.xFramerate = 0;
  //set changes
  OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));
  
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
  
  render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
					   |OMX_DISPLAY_SET_FULLSCREEN
					   |OMX_DISPLAY_SET_NOASPECT
					   |OMX_DISPLAY_SET_MODE);
  render_config.fullscreen = OMX_FALSE;
  render_config.noaspect = OMX_FALSE;
 
  render_config.dest_rect.width = screen_width/2;
  render_config.dest_rect.height = screen_height;

  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));  
  
  /*
  DOES NOT WORK LEAVING AS A REMINDER

  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90; 
  render_config.set = OMX_DISPLAY_SET_DUMMY;
  
  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Config. Error = %s\n", err2str(OMXstatus));  
  print_OMX_CONFIG_DISPLAYREGIONTYPE(render_config);
  */



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

  //populate image_format with information from the camera port
  image_format.nPortIndex = 72;

  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Paramter. Error = %s\n", err2str(OMXstatus));

  //change some settings and set parameters
  image_format.eCompressionFormat = OMX_IMAGE_CodingJPEG;
  image_format.nPortIndex = 341;
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);
  
  OMXstatus = OMX_SetParameter(ilclient_get_handle(image_encode), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error setting Paramter. Error = %s\n", err2str(OMXstatus));

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

  
  //tell API port is taking picture
  still_capture_in_progress.bEnabled = 1;
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
      //printf("decode_out bytes = %d\n", decode_out->nFilledLen);
      //printf("decode_out bufferflags = %d\n\n", decode_out->nFlags);
      if(decode_out->nFilledLen != 0) 
	fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out1);
      if(decode_out->nFlags == 1)
	break;
      OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
    }
    
  
  //tell API port is finished capture
  still_capture_in_progress.bEnabled = 0;
  still_capture_in_progress.nVersion.nVersion = OMX_VERSION;
  still_capture_in_progress.nSize = sizeof(still_capture_in_progress);
  still_capture_in_progress.nPortIndex = 72;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera),
			       OMX_IndexConfigPortCapturing,
			       &still_capture_in_progress);  
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to set Config (1)\n");
      exit(EXIT_FAILURE);
    }
  
  printf("captureSaved\n");
  //sleep for 2 secs
  sleep(2);
  

  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //close files
  fclose(file_out1);
  
  //Disable components
  

  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  return 0;
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}
