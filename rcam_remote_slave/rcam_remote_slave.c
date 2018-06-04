/*
Remote Camera

Must accept commands from the main Pi

Commands should include
   Take Photo
   Request Preview Resolution (include in start preview)
   Start Preview
   Stop Preview

Desirable commands - spolier not implemented due to bad omx implementation
   White Balence
   Other camera Options

NOTE this will not be portable code (due to it realying both ends to have the same endieness) !

Even while displaying preview remote camera should accept and process commands
The commands will be identified by an integer followed by the relevent(if any) information to be processed
when no command is needed the main pi will send NO_COMMAND the commands will use the same TCP connection
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> //used for sleep()

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "print_OMX.h"
#include "socket_helper.h"
#include "rcam.h"

//#define IP_ADD "127.0.0.1" //loopback ip
#define IP_ADD "192.168.0.21"
#define SERV_PORT "8039"

////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
////////////////////////////////////////////////////////////////

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data);

////////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    int count = 0;
    int socket_fd;
    enum rcam_command current_command;
    bool deliver_preview = false;

    ILCLIENT_T *client;
    COMPONENT_T *camera = NULL, *video_scheduler = NULL, *image_encode = NULL;
    OMX_ERRORTYPE OMXstatus;

    TUNNEL_T tunnel_camera_to_scheduler, tunnel_camera_to_encode;
    memset(&tunnel_camera_to_scheduler, 0, sizeof(tunnel_camera_to_scheduler));
    memset(&tunnel_camera_to_encode, 0, sizeof(tunnel_camera_to_encode));


    //INITIALIZE CAMERA STUFF

    //initialize bcm host
    bcm_host_init();

    //create client
    client = ilclient_init();
    if(client == NULL)
    {
        fprintf(stderr, "unable to initialize ilclient\n");
        exit(EXIT_FAILURE);
    }

    //initialize OMX
    OMXstatus = OMX_Init();
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to initialize OMX");
        ilclient_destroy(client);
        exit(EXIT_FAILURE);
    }

    // set error callback
    ilclient_set_error_callback(client,
                                error_callback,
                                NULL);

    ////////////////////////////////
    //Loop until connected to server program
    while(1)
      {
	socket_fd = getAndConnectTCPSocket(IP_ADD, SERV_PORT);
	if (socket_fd < 0)
	  {
	    printf("socket failure\n");
	    sleep(1);
	  }
	else
	  {
	    printf("socket success!, socket_fd = %d\n", socket_fd);
	    break;
	  }
      }

    /////////////////////////////////
    // SEND AND RECV

    char handshake_r[4];
    long int num_bytes = 0;
    int previewWidth, previewHeight, previewFramerate;
    int captureWidth, captureHeight;
    OMX_BUFFERHEADERTYPE *previewHeader;

    //handshake
    printf("sending handshake\n");
    write(socket_fd, "handshake\0", sizeof(char) * 11);
    printf("handshake sent\n");
    read(socket_fd, &handshake_r, sizeof(char)*4);
    printf("handshake = %s\n\n", handshake_r);

    read(socket_fd, &previewWidth, sizeof(previewWidth));
    read(socket_fd, &previewHeight, sizeof(previewHeight));
    read(socket_fd, &previewFramerate, sizeof(previewFramerate));

    read(socket_fd, &captureWidth, sizeof(captureWidth));
    read(socket_fd, &captureHeight, sizeof(captureHeight));

    printf("rcam_remote_slave recived values:\n");
    printf("preview %d x %d   framerate %d   ", previewWidth, previewHeight, previewFramerate);
    printf("capture %d x %d\n", captureWidth, captureHeight);

    ////////////////////////////
    //initialize camera
    printf("\n\n---initalize camera component---\n");
    
    
    ilclient_create_component(client,
                              &camera,
                              "camera",
                              ILCLIENT_DISABLE_ALL_PORTS
			      );
    
    printState(ilclient_get_handle(camera));

    OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Idle (1)");
        exit(EXIT_FAILURE);
    }


    //defaults
    //set the capture resolution
    setCaptureRes(camera, captureWidth, captureHeight);
    //set default preview resolution
    setPreviewRes(camera, previewWidth, previewHeight, previewFramerate);
    
    ////////////////////////
    ////Initalize Video Scheduler
    printf("---initalize video_scheduler component---\n");
    
    ilclient_create_component(client,
			      &video_scheduler,
			      "video_scheduler",
			      ILCLIENT_DISABLE_ALL_PORTS
			      | ILCLIENT_ENABLE_OUTPUT_BUFFERS
			      );
    
    OMXstatus = ilclient_change_component_state(video_scheduler, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move scheduler component to Idle (1)");
        exit(EXIT_FAILURE);
    }
    
    ////////////////////////
    ////Initialize Image Encoder
    printf("---initalize image_encode component---\n");
    
    
    ilclient_create_component(client,
			      &image_encode,
			      "image_encode",
			      ILCLIENT_DISABLE_ALL_PORTS			      
			      | ILCLIENT_ENABLE_OUTPUT_BUFFERS
			      );

    OMXstatus = ilclient_change_component_state(image_encode, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
      {
	fprintf(stderr, "unable to move image encode component to Idle (1)\n");
	exit(EXIT_FAILURE);
      }

    //image format Param set
    setParamImageFormat(image_encode, JPEG_HIGH_FORMAT);

    ////////////////////////
    ////enable tunnels
    
    set_tunnel(&tunnel_camera_to_scheduler, camera, 70, video_scheduler, 10);
    ilclient_setup_tunnel(&tunnel_camera_to_scheduler, 0, 0);

    // change camera to executing
    OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
    if (OMXstatus != OMX_ErrorNone)
      {
	fprintf(stderr, "unable to move camera component to Executing (1)\n");
	exit(EXIT_FAILURE);
      }

    //enable port and buffers for video_scheduler
    ilclient_enable_port_buffers(video_scheduler, 11, NULL, NULL, NULL);
    ilclient_enable_port(video_scheduler, 11);

    // change video scheduler to executing
    OMXstatus = ilclient_change_component_state(video_scheduler, OMX_StateExecuting);
    if (OMXstatus != OMX_ErrorNone)
      {
	fprintf(stderr, "unable to move video_scheduler component to Executing (1)\n");
	exit(EXIT_FAILURE);
      }

    
  //enable port and buffers for output port of image encode
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


    
    
    while(1)
      {
	count++;
	//printf("count = %d\n", count);

	//get command
	//printf("waiting for command ... \n IN RCAM\n\n\n");
	read(socket_fd, &current_command, sizeof(current_command));
	//printf("got command = %d\n", (int)current_command);

	if (current_command == START_PREVIEW)
	  {
	    deliver_preview = true;
	  }
	else if (current_command == SET_PREVIEW_RES)
	  {
	    //get the values
	    read(socket_fd, &previewWidth, sizeof(previewWidth));
	    read(socket_fd, &previewHeight, sizeof(previewHeight));
	    read(socket_fd, &previewFramerate, sizeof(previewFramerate));
	    printf("previewWidth = %d\n", previewWidth);
	    printf("previewHeight = %d\n", previewHeight);
	    printf("framerate = %d\n", previewFramerate);
	    //pause component
	    ilclient_change_component_state(camera, OMX_StatePause);
	    printState(ilclient_get_handle(camera));
	    OMXstatus = OMX_SendCommand(ilclient_get_handle(camera), OMX_CommandFlush, 70, NULL);
	    if (OMXstatus != OMX_ErrorNone)
	      {
		fprintf(stderr, "Error = %s\n", err2str(OMXstatus));
		exit(EXIT_FAILURE);
	      }

	    printf("command sent ");
	    ilclient_wait_for_event(camera, OMX_EventCmdComplete, OMX_CommandFlush, 0, 70, 0,
				    ILCLIENT_PORT_FLUSH, VCOS_EVENT_FLAGS_SUSPEND);
	    printf("flsuhed port ");
	    ilclient_disable_port_buffers(camera, 70, NULL, NULL, NULL);
      	    printf("disabled ports buffers\n");
	    //change the preview port
	    setPreviewRes(camera, previewWidth, previewHeight, previewFramerate);
    	    ilclient_enable_port_buffers(camera, 70, NULL, NULL, NULL);
	    printf("enabled ports\n");

	    ilclient_change_component_state(camera, OMX_StateExecuting);

	  }
	else if (current_command == SET_CAPTURE_RES)
	  {
	    //get the values
	    //change the capture port
	    //send the value back?
	  }
	else if (current_command == TAKE_PHOTO)
	  {
	    savePhoto(camera, image_encode, "photo_R");
	    //TODO close file and open next!
	    
	
	    /* THIS WAS NOT WORKING BUT NOT PREPARED TO KILL IT QUITE YET
	    //2 options save locally
	    printf("In rcam_remote_slave take photo\n");
	    
	    OMX_ERRORTYPE OMXstatus;
	    OMX_BUFFERHEADERTYPE *camera_capture_out, *decode_in, *decode_out;

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

	    printf("enable capture port\n");
	    ilclient_enable_port(camera, 72); 
	    
	    while(1)
	      {
		printf("in while loop \n");
		printf(".get output buffer\n");
		camera_capture_out = ilclient_get_output_buffer(camera, 72, 1);
		printf(".empty the buffer");
		OMX_EmptyThisBuffer(ilclient_get_handle(camera), camera_capture_out);
		
		//copy buffer DO camera_capture_out to decode_in
		printf(".memcpy\n");
		memcpy(camera_capture_out, decode_in, sizeof(camera_capture_out));

		printf(".get input buffer");
		decode_in = ilclient_get_input_buffer(image_encode, 340, 1);
		printf(".fill buffer");
		OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_in);
		
		decode_out = ilclient_get_output_buffer(image_encode, 341, 1);
		printf("decode_out bytes = %d   :   ", decode_out->nFilledLen);
		printf("decode_out bufferflags = %d\n", decode_out->nFlags);

		if(decode_out->nFilledLen != 0)
		  {
		    fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out2);

		  }
		if(decode_out->nFlags == 1)
		  {
		    fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out2);
		    OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
		    break;
		  }
		//crashes if starts with 0 buffer?
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

	    ilclient_disable_port(camera, 72); 	    

	    printf("captureSaved\n");
	    printf("save photo done\n");
	    //current_command == NO_COMMAND;
	    //or
	    //store in a buffer
	    //send size of buffer
	    //send buffer
	    */
	  }
	else if (current_command == END_REMOTE_CAM)
	  {
	    break;
	  }
	else if (current_command == NO_COMMAND)
	  {
	    if (deliver_preview = true)
	      {
		//print state (to check its still executing
		printState(ilclient_get_handle(camera));

		//get buffer from video_scheduler		
		OMXstatus = OMX_FillThisBuffer(ilclient_get_handle(video_scheduler), previewHeader);
		previewHeader = ilclient_get_output_buffer(video_scheduler, 11, 1);		
		//send buffer, checks lengths to ensure all data is sent
		printf("nAllocLen = %d\n", previewHeader->nAllocLen);
		printf("sending buffer ... ");
		num_bytes = write(socket_fd, previewHeader->pBuffer,  previewHeader->nAllocLen);
		while (num_bytes < previewHeader->nAllocLen)
		  num_bytes += write(socket_fd,
				     previewHeader->pBuffer + num_bytes,
				     previewHeader->nAllocLen - num_bytes);
		printf("buffer sent, %ld bytes \n", num_bytes);
	      }
	  }
      }//end of while loop


    //make camera idle again
    OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Idle (1)");
        exit(EXIT_FAILURE);
    }
    printState(ilclient_get_handle(camera));

    printf("exiting remote_cam.c");
    return 0;
}


/////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////

//error callback for OMX
void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}
