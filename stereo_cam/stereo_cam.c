
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
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "bcm_host.h"
#include "ilclient.h"

//defines
#define PORT "8039"

//move to a header for remote_cam
enum rcam_command
{
    NO_COMMAND = 0,
    SET_PREVIEW_RES = 10,
    SET_PREVIEW_FRAMERATE = 11,
    START_PREVIEW = 20,
    STOP_PREVIEW = 21,
    TAKE_PHOTO = 30
};

/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////

void print_OMX_AUDIO_PORTDEFINITIONTYPE(OMX_AUDIO_PORTDEFINITIONTYPE audio)
{
  /*
  printf("   audio.cMIMEType = %s\n", audio.cMIMEType);
  printf("   audio.pNativeRender = %d\n", (int)audio.pNativeRender);
  printf("   audio.bFlagErrorConcealment = ");
  audio.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
  printf("   audio.eEncoding = %d (enumeration not implemented)\n", (int)audio.eEncoding);
  */
  printf("Not implemented yet\n");
}

void print_OMX_VIDEO_PORTDEFINITIONTYPE(OMX_VIDEO_PORTDEFINITIONTYPE video)
{
  printf("OMX_VIDEO_PORT_DEFINITIONTYPE\n");
  printf("   .video.cMIMEType = %s\n", video.cMIMEType);
  printf("   .video.pNativeRender = %d ??\n", (int)video.pNativeRender);
  printf("   .video.nFrameWidth = %d\n", (int)video.nFrameWidth);
  printf("   .video.nFrameHeight = %d\n", (int)video.nFrameHeight);
  printf("   .video.nStride = %d\n", (int)video.nStride);
  printf("   .video.nSliceHeight = %d\n", (int)video.nSliceHeight);
  printf("   .video.nBitrate = %d\n", (int)video.nBitrate);
  printf("   .video.xFramerate >> 16 = %d\n", (int)video.xFramerate >>16 );
  printf("   .video.bFlagErrorConcealment = ");
  video.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
  printf("   .video.eCompressionFormat = ");
  switch(video.eCompressionFormat)
    {
    case OMX_VIDEO_CodingUnused:     printf("OMX_VIDEO_CodingUnused\n"); break;
    case OMX_VIDEO_CodingAutoDetect: printf("OMX_VIDEO_CodingAutoDetect\n"); break;
    case OMX_VIDEO_CodingMPEG2:      printf("OMX_VIDEO_CodingMPEG2\n"); break;
    case OMX_VIDEO_CodingH263:       printf("OMX_VIDEO_CodingH263\n"); break;
    case OMX_VIDEO_CodingMPEG4:      printf("OMX_VIDEO_CodingMPEG4\n"); break;
    case OMX_VIDEO_CodingWMV:        printf("OMX_VIDEO_CodingWMV\n"); break;
    case OMX_VIDEO_CodingRV:         printf("OMX_VIDEO_CodingRV\n"); break;
    case OMX_VIDEO_CodingAVC:        printf("OMX_VIDEO_CodingAVC\n"); break;
    case OMX_VIDEO_CodingMJPEG:      printf("OMX_VIDEO_CodingMJPEG\n"); break;
    case OMX_VIDEO_CodingMax:        printf("OMX_VIDEO_CodingMax (not a video format)\n"); break;
    default: printf("Format not accounted for"); break;
    }
  switch(video.eColorFormat)
    {
    case OMX_COLOR_FormatUnused:                 printf("OMX_COLOR_FormatUnused\n"); break;
    case OMX_COLOR_FormatMonochrome:             printf("OMX_COLOR_FormatMonochrome\n"); break;
    case OMX_COLOR_Format8bitRGB332:             printf("OMX_COLOR_Format8bitRGB332\n"); break;
    case OMX_COLOR_Format12bitRGB444:            printf("OMX_COLOR_Format12bitRGB444\n"); break;
    case OMX_COLOR_Format16bitARGB4444:          printf("OMX_COLOR_Format16bitARGB4444\n"); break;
    case OMX_COLOR_Format16bitARGB1555:          printf("OMX_COLOR_Format16bitARGB1555\n"); break;
    case OMX_COLOR_Format16bitRGB565:            printf("OMX_COLOR_Format16bitRGB565\n"); break;
    case OMX_COLOR_Format16bitBGR565:            printf("OMX_COLOR_Format16bitBGR565\n"); break;
    case OMX_COLOR_Format18bitRGB666:            printf("OMX_COLOR_Format18bitRGB666\n"); break;
    case OMX_COLOR_Format18bitARGB1665:          printf("OMX_COLOR_Format18bitARGB1665\n"); break;
    case OMX_COLOR_Format19bitARGB1666:          printf("OMX_COLOR_Format19bitARGB1666\n"); break;
    case OMX_COLOR_Format24bitRGB888:            printf("OMX_COLOR_Format24bitRGB888\n"); break;
    case OMX_COLOR_Format24bitBGR888:            printf("OMX_COLOR_Format24bitBGR888\n"); break;
    case OMX_COLOR_Format24bitARGB1887:          printf("OMX_COLOR_Format24bitARGB1887\n"); break;
    case OMX_COLOR_Format25bitARGB1888:          printf("OMX_COLOR_Format25bitARGB1888\n"); break;
    case OMX_COLOR_Format32bitBGRA8888:          printf("OMX_COLOR_Format32bitBGRA8888\n"); break;
    case OMX_COLOR_Format32bitARGB8888:          printf("OMX_COLOR_Format32bitARGB8888\n"); break;
    case OMX_COLOR_FormatYUV411Planar:           printf("OMX_COLOR_FormatYUV411Planar\n"); break;
    case OMX_COLOR_FormatYUV411PackedPlanar:     printf("OMX_COLOR_FormatYUV411PackedPlanar\n"); break;
    case OMX_COLOR_FormatYUV420Planar:           printf("OMX_COLOR_FormatYUV420Planar\n"); break;
    case OMX_COLOR_FormatYUV420PackedPlanar:     printf("OMX_COLOR_FormatYUV420PackedPlanar\n"); break;
    case OMX_COLOR_FormatYUV420SemiPlanar:       printf("OMX_COLOR_FormatYUV420SemiPlanar\n"); break;
    case OMX_COLOR_FormatYUV422Planar:           printf("OMX_COLOR_FormatYUV422Planar\n"); break;
    case OMX_COLOR_FormatYUV422PackedPlanar:     printf("OMX_COLOR_FormatYUV422PackedPlanar\n"); break;
    case OMX_COLOR_FormatYUV422SemiPlanar:       printf("OMX_COLOR_FormatYUV422SemiPlanar\n"); break;
    case OMX_COLOR_FormatYCbYCr:                 printf("OMX_COLOR_FormatYCbYCr\n"); break;
    case OMX_COLOR_FormatYCrYCb:                 printf("OMX_COLOR_FormatYCrYCb\n"); break;
    case OMX_COLOR_FormatCbYCrY:                 printf("OMX_COLOR_FormatCbYCrY\n"); break;
    case OMX_COLOR_FormatCrYCbY:                 printf("OMX_COLOR_FormatCrYCbY\n"); break;
    case OMX_COLOR_FormatYUV444Interleaved:      printf("OMX_COLOR_FormatYUV444Interleaved\n"); break;
    case OMX_COLOR_FormatRawBayer8bit:           printf("OMX_COLOR_FormatRawBayer8bit\n"); break;
    case OMX_COLOR_FormatRawBayer10bit:          printf("OMX_COLOR_FormatRawBayer10bit\n"); break;
    case OMX_COLOR_FormatRawBayer8bitcompressed: printf("OMX_COLOR_FormatRawBayer8bitcompressed\n"); break;
    case OMX_COLOR_FormatL2:                     printf("OMX_COLOR_FormatL2\n"); break;
    case OMX_COLOR_FormatL4:                     printf("OMX_COLOR_FormatL4\n"); break;
    case OMX_COLOR_FormatL8:                     printf("OMX_COLOR_FormatL8\n"); break;
    case OMX_COLOR_FormatL16:                    printf("OMX_COLOR_FormatL16\n"); break;
    case OMX_COLOR_FormatL24:                    printf("OMX_COLOR_FormatL24\n"); break;
    case OMX_COLOR_FormatL32:                    printf("OMX_COLOR_FormatL32\n"); break;
    case OMX_COLOR_FormatYUV420PackedSemiPlanar: printf("OMX_COLOR_FormatYUV420PackedSemiPlanar\n"); break;
    case OMX_COLOR_FormatYUV422PackedSemiPlanar: printf("OMX_COLOR_FormatYUV422PackedSemiPlanar\n"); break;
    case OMX_COLOR_Format18BitBGR666:            printf("OMX_COLOR_Format18BitBGR666\n"); break;
    case OMX_COLOR_Format24BitARGB6666:          printf("OMX_COLOR_Format24BitARGB6666\n"); break;
    case OMX_COLOR_Format24BitABGR6666:          printf("OMX_COLOR_Format24BitABGR6666\n"); break;
    case OMX_COLOR_FormatMax:                    printf("OMX_COLOR_FormatMax (not a format)\n"); break;
    default: printf("Format not accounted for\n"); break;
    }
  printf("pNativeWindow = %d\n", (int)video.pNativeWindow);
}

void print_OMX_IMAGE_PORTDEFINITIONTYPE(OMX_IMAGE_PORTDEFINITIONTYPE image)
{
  printf("Not implemented yet\n");
}

void print_OMX_OTHER_PORTDEFINITIONTYPE(OMX_OTHER_PORTDEFINITIONTYPE other)
{
  printf("Not implemented yet\n");
}

//prints port params
void print_OMX_PARAM_PORTDEFINITIONTYPE(OMX_PARAM_PORTDEFINITIONTYPE params)
{
  printf("-----------------------\n");
  printf("nSize = %d\n", (int)params.nSize);
  printf("nVersion.nVersion.nVersionMajor = %d\n", (int)params.nVersion.s.nVersionMajor);
  printf("                 .nVersionMinor = %d\n", (int)params.nVersion.s.nVersionMinor);
  printf("                 .nReviseion = %d\n",    (int)params.nVersion.s.nRevision);
  //printf("                 .nStep = &d\n",         (int)params.nVersion.s.nStep);
  printf("nPortIndex = %d\n", params.nPortIndex);
  switch(params.eDir)
    {
    case OMX_DirInput: printf("eDir = OMX_DirInput (Input port)\n"); break;
    case OMX_DirOutput: printf("eDir = OMX_DirOutput (Output port)\n"); break;
    case OMX_DirMax: printf("eDir = OMX_DirMax (?)\n"); break;
    default: printf("Possible Error: unable to determin direction of port\n");
    } 
  printf("nBufferCountActual = %d\n", (int)params.nBufferCountActual);
  printf("nBufferCountMin = %d\n", (int)params.nBufferCountMin);
  printf("nBufferSize = %d\n", (int)params.nBufferSize);
  printf("bEnabled = "); params.bEnabled ? printf("true\n") : printf("false\n");
  printf("bPopulated = "); params.bPopulated ? printf("true\n") : printf("false\n");
  switch(params.eDomain)
    {
    case OMX_PortDomainAudio:
      printf("eDomain = OMX_PortDomainAudio (Audio Port)\n");
      print_OMX_AUDIO_PORTDEFINITIONTYPE(params.format.audio);
      break;
    case OMX_PortDomainVideo:
      printf("eDomain = OMX_PortDomainVideo (Video Port)\n");
      print_OMX_VIDEO_PORTDEFINITIONTYPE(params.format.video);
      break;
    case OMX_PortDomainImage:
      printf("eDomain = OMX_PortDomainImage (Image Port)\n");
      print_OMX_IMAGE_PORTDEFINITIONTYPE(params.format.image);
      break;
    case OMX_PortDomainOther:
      printf("eDomain = OMX_PortDomainOther (Other Port)\n");
      print_OMX_OTHER_PORTDEFINITIONTYPE(params.format.other);
      break;
    default:
      printf("eDomain = is another type not accounted for (?)\n");
      break;
    }


  printf("bBuffersContiguous = "); params.bBuffersContiguous ? printf("true\n") : printf("false\n");
  printf("nBufferAlignment = %d\n", (int)params.nBufferAlignment);
  
  printf("-----------------------\n");
}

void printBits(void *toPrint);

void printState(OMX_HANDLETYPE handle);

void print_OMX_CONFIG_DISPLAYREGIONTYPE(OMX_CONFIG_DISPLAYREGIONTYPE current);

char *err2str(int err);

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data);


/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  /////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////
  printf("sterocam started\n");

  ILCLIENT_T *client;
  COMPONENT_T *video_render = NULL;

  OMX_ERRORTYPE OMXstatus;

  uint32_t screen_width = 0, screen_height = 0;
  OMX_BUFFERHEADERTYPE *video_render_in;

  int numbytes;
  char char_buffer[12];

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

  /////////////////////////////////////////////////////////////////
  // SOCKET STUFF
  /////////////////////////////////////////////////////////////////
  printf("start of socket stuff");
  
  //socket stuctures and vars and stuff
  int socket_fd, new_sock;
  struct addrinfo hints, *results;
  struct sockaddr_storage remote_cam_addr;
  socklen_t addr_size = sizeof(remote_cam_addr);
  int socket_status = 0, result;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // getaddrinfo
  socket_status = getaddrinfo(NULL, PORT, &hints, &results);
  if(socket_status != 0)
    {
      fprintf(stderr, "getaddrinfo failed, error = %s\n", gai_strerror(socket_status));
      exit(EXIT_FAILURE);
    }

  printf("results->ai_protocol = %d", results->ai_protocol);

  // socket
  socket_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(socket_fd <= 0)
    {
      fprintf(stderr, "socket failed\n");
      exit(EXIT_FAILURE);
    }

  // bind
  socket_status = bind(socket_fd, results->ai_addr, results->ai_addrlen);
  if(socket_status != 0)
    {
      fprintf(stderr, "bind failed\n");
      exit(EXIT_FAILURE);
    }

  freeaddrinfo(results);

  printf("waiting for remotecam to connect");
  
  //listen
  if (listen(socket_fd, 10) == -1)
    {
      fprintf(stderr, "listen failed");
      exit(EXIT_FAILURE);
    }

  //accept
  new_sock = accept(socket_fd, (struct sockaddr *) &remote_cam_addr, &addr_size); 
  if(new_sock < 0)
    printf("error accepting socket, error = %s", strerror(errno));

  printf("socket = %d\n", socket_fd);
  printf("new_sock = %d", new_sock);
  /////////////////////////////////////////////////////////////////
  // FORK
  /////////////////////////////////////////////////////////////////
#ifdef FORK

  pid_t pid;

  pid = fork();
  if (pid == 0)
    printf("in child process!!!\n");  //call a fuction etc
  if (pid < 0)
    fprintf(stderr, "Fork failed!\n");
  else /* if pid > 0 */
    printf("in main process\n");

#endif

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
  ////Initialise video render////
  ///////////////////////////////////////////
  ilclient_create_component(client,
			    &video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_INPUT_BUFFERS);

  OMXstatus = ilclient_change_component_state(video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move render component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }

  //set the port params to the same as the remote camera

  int width = 320, height = 240;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  render_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  render_params.format.video.nFrameWidth = width;
  render_params.format.video.nFrameHeight = height;
  render_params.format.video.nStride = width;
  render_params.format.video.nSliceHeight = height;
  render_params.format.video.xFramerate = 24 << 16;

  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting video render port parameters (1)");

  //check the port params
  memset(&render_params, 0, sizeof(render_params));
  render_params.nVersion.nVersion = OMX_VERSION;
  render_params.nSize = sizeof(render_params);
  render_params.nPortIndex = 90;

  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting video render port parameters (1)");

  print_OMX_PARAM_PORTDEFINITIONTYPE(render_params);

  //set the position on the screen
  /*
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
  */

  //ask ilclient to allocate buffers for video_render
  printf("enable video_render_input port\n");
  ilclient_enable_port_buffers(video_render, 90, NULL, NULL,  NULL);
  ilclient_enable_port(video_render, 90);


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


  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////

  //change preview render to executing
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printf("video_render state is ");
  printState(ilclient_get_handle(video_render));
  printf("***\n");

  ////////////////////////////////////////////////////////////
  // SEND AND RECV
  ////////////////////////////////////////////////////////////
  
  //handshake
  printf("waiting to recive handshake ... \n");
  read(new_sock, char_buffer, 11);
  printf("handshake result = %s", char_buffer);
  write(new_sock, "got\0", sizeof(char)*4);

  void * temp_buffer;
  temp_buffer = malloc(render_params.nBufferSize + 1 );  

  int count = 0;
  long int num_bytes = 0;
  enum rcam_command current_command = START_PREVIEW; 

  printf("current_command = %d\n", current_command);

  printf("sending command ...");
  write(new_sock, &current_command, sizeof(current_command));
  printf("sent command\n");
  
  current_command = NO_COMMAND;

  printf("*** nBufferSize = %d\n", render_params.nBufferSize);

  while(count < 100)
    {
      printState(ilclient_get_handle(video_render));
      count++;
      printf("count = %d\n", count);

      printf("get a buffer to process\n");
      printf("waiting to recv buffer of size %d... ", render_params.nBufferSize);
      num_bytes = read(new_sock,
		       temp_buffer,
		       render_params.nBufferSize);
      while (num_bytes < render_params.nBufferSize)
	{
	  num_bytes += read(new_sock,
			    temp_buffer + num_bytes,
			    render_params.nBufferSize - num_bytes);
	}
      printf("buffer recived, recived %ld bytes\n", num_bytes);
      
      //change nAllocLen in bufferheader
      video_render_in = ilclient_get_input_buffer(video_render, 90, 1);
      memcpy(video_render_in->pBuffer, temp_buffer, render_params.nBufferSize);
      printf("copied buffer form temp into video_render_in\n");
      //fix alloc len
      video_render_in->nFilledLen = render_params.nBufferSize;

      //empty buffer into render component
      OMX_EmptyThisBuffer(ilclient_get_handle(video_render), video_render_in);
      printf("Emptied buffer\n");

      //send no command
      write(new_sock, &current_command, sizeof(current_command));      
    }
      
  
  putchar('\n');

  //sleep for 2 secs
  sleep(2);


  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //free buffer memory
  free(temp_buffer);
  
  //Disable components


  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  return 0;
}


char *err2str(int err) {
  switch (err) {
  case OMX_ErrorInsufficientResources: return "OMX_ErrorInsufficientResources";
  case OMX_ErrorUndefined: return "OMX_ErrorUndefined";
  case OMX_ErrorInvalidComponentName: return "OMX_ErrorInvalidComponentName";
  case OMX_ErrorComponentNotFound: return "OMX_ErrorComponentNotFound";
  case OMX_ErrorInvalidComponent: return "OMX_ErrorInvalidComponent";
  case OMX_ErrorBadParameter: return "OMX_ErrorBadParameter";
  case OMX_ErrorNotImplemented: return "OMX_ErrorNotImplemented";
  case OMX_ErrorUnderflow: return "OMX_ErrorUnderflow";
  case OMX_ErrorOverflow: return "OMX_ErrorOverflow";
  case OMX_ErrorHardware: return "OMX_ErrorHardware";
  case OMX_ErrorInvalidState: return "OMX_ErrorInvalidState";
  case OMX_ErrorStreamCorrupt: return "OMX_ErrorStreamCorrupt";
  case OMX_ErrorPortsNotCompatible: return "OMX_ErrorPortsNotCompatible";
  case OMX_ErrorResourcesLost: return "OMX_ErrorResourcesLost";
  case OMX_ErrorNoMore: return "OMX_ErrorNoMore";
  case OMX_ErrorVersionMismatch: return "OMX_ErrorVersionMismatch";
  case OMX_ErrorNotReady: return "OMX_ErrorNotReady";
  case OMX_ErrorTimeout: return "OMX_ErrorTimeout";
  case OMX_ErrorSameState: return "OMX_ErrorSameState";
  case OMX_ErrorResourcesPreempted: return "OMX_ErrorResourcesPreempted";
  case OMX_ErrorPortUnresponsiveDuringAllocation: return "OMX_ErrorPortUnresponsiveDuringAllocation";
  case OMX_ErrorPortUnresponsiveDuringDeallocation: return "OMX_ErrorPortUnresponsiveDuringDeallocation";
  case OMX_ErrorPortUnresponsiveDuringStop: return "OMX_ErrorPortUnresponsiveDuringStop";
  case OMX_ErrorIncorrectStateTransition: return "OMX_ErrorIncorrectStateTransition";
  case OMX_ErrorIncorrectStateOperation: return "OMX_ErrorIncorrectStateOperation";
  case OMX_ErrorUnsupportedSetting: return "OMX_ErrorUnsupportedSetting";
  case OMX_ErrorUnsupportedIndex: return "OMX_ErrorUnsupportedIndex";
  case OMX_ErrorBadPortIndex: return "OMX_ErrorBadPortIndex";
  case OMX_ErrorPortUnpopulated: return "OMX_ErrorPortUnpopulated";
  case OMX_ErrorComponentSuspended: return "OMX_ErrorComponentSuspended";
  case OMX_ErrorDynamicResourcesUnavailable: return "OMX_ErrorDynamicResourcesUnavailable";
  case OMX_ErrorMbErrorsInFrame: return "OMX_ErrorMbErrorsInFrame";
  case OMX_ErrorFormatNotDetected: return "OMX_ErrorFormatNotDetected";
  case OMX_ErrorContentPipeOpenFailed: return "OMX_ErrorContentPipeOpenFailed";
  case OMX_ErrorContentPipeCreationFailed: return "OMX_ErrorContentPipeCreationFailed";
  case OMX_ErrorSeperateTablesUsed: return "OMX_ErrorSeperateTablesUsed";
  case OMX_ErrorTunnelingUnsupported: return "OMX_ErrorTunnelingUnsupported";
  default: return "unknown error";
  }
}

void printBits(void *toPrint)
{
  unsigned char *b = (unsigned char*) toPrint;
  unsigned char byte;
  int i, j;
  size_t size = sizeof(*toPrint);

  for (i = size -1; i >= 0; i--)
    {
      for (j = 7; j >= 0; j--)
	{
	  byte = b[i] & (1<<j);
	  byte >>= j;
	  printf("%u", byte);
	}
    }
}

void print_OMX_CONFIG_DISPLAYREGIONTYPE(OMX_CONFIG_DISPLAYREGIONTYPE current)
{
  printf("*********************************************\n");
  printf("nSize = %d\n", current.nSize );
  printf("nVersion.nVersion = %d\n", current.nVersion.nVersion );
  printf("nPortIndex = %d\n", current.nPortIndex );

  putchar('\n');
  printf("set = ");
  printBits(&current.set);
  putchar('\n');

  printf("OMX_DISPLAY_SET_NONE = %d\n", (current.set >> 0) & 1);
  printf("OMX_DISPLAY_SET_NUM = %d\n", (current.set >> 1) & 1);
  printf("OMX_DISPLAY_SET_FULLSCREEN = %d\n", (current.set >> 2) & 1);
  printf("OMX_DISPLAY_SET_TRANSFORM = %d\n", (current.set >> 3) & 1);
  printf("OMX_DISPLAY_SET_DEST_RECT = %d\n", (current.set >> 4) & 1);
  printf("OMX_DISPLAY_SET_SRC_RECT = %d\n", (current.set >> 5) & 1);
  printf("OMX_DISPLAY_SET_MODE = %d\n", (current.set >> 6) & 1);
  printf("OMX_DISPLAY_SET_PIXEL = %d\n", (current.set >> 7) & 1);
  printf("OMX_DISPLAY_SET_NOASPECT = %d\n", (current.set >> 8) & 1);
  printf("OMX_DISPLAY_SET_LAYER = %d\n", (current.set >> 9) & 1);
  printf("OMX_DISPLAY_SET_COPYPROTECT = %d\n", (current.set >> 10) & 1);
  printf("OMX_DISPLAY_SET_ALPHA = %d\n", (current.set >> 11) & 1);
  printf("OMX_DISPLAY_SET_DUMMY = %d\n", current.set == OMX_DISPLAY_SET_DUMMY);

  putchar('\n');

  printf("num = %d\n", current.num );
  printf("fullscreen = %d\n", current.fullscreen );

  printf("transform = ");
  switch(current.transform)
    {
    case 0: printf("OMX_DISPLAY_ROT0   (0)\n"); break;
    case 1: printf("OMX_DISPLAY_MIRROR_ROT0   (1)\n"); break;
    case 2: printf("OMX_DISPLAY_MIRROR_ROT180   (2)\n"); break;
    case 3: printf("OMX_DISPLAY_ROT180   (3)\n"); break;
    case 4: printf("OMX_DISPLAY_MIRROR_ROT90   (4)\n"); break;
    case 5: printf("OMX_DISPLAY_ROT270   (5)\n"); break;
    case 6: printf("OMX_DISPLAY_ROT90   (6)\n"); break;
    case 7: printf("OMX_DISPLAY_MIRROR_ROT270   (7)\n"); break;
    case 0x7FFFFFFF: printf("OMX_DISPLAY_DUMMY   (0x7FFFFFFF)\n"); break;
    default : printf("Error: Enumeration unknown"); break;
    }

  putchar('\n');

  printf("dest_rect\n");
  printf("\tdest_rect.x_offset = %d\n", current.dest_rect.x_offset);
  printf("\tdest_rect.y_offset = %d\n", current.dest_rect.y_offset);
  printf("\tdest_rext.width = %d\n", current.dest_rect.width);
  printf("\tdest_rext.height = %d\n", current.dest_rect.height);

  putchar('\n');

  printf("src_rect\n");
  printf("\tsrc_rect.x_offset = %d\n", current.src_rect.x_offset);
  printf("\tsrc_rect.y_offset = %d\n", current.src_rect.y_offset);
  printf("\tsrc_rext.width = %d\n", current.src_rect.width);
  printf("\tsrc_rext.height = %d\n", current.src_rect.height);

  putchar('\n');

  printf("noaspect = %d\n", current.noaspect );
  printf("pixel_x = %d\n", current.pixel_x );
  printf("pixel_y = %d\n", current.pixel_y );
  printf("layer = %d\n", current.layer );
  printf("copyprotect_required = %d\n", current.copyprotect_required );
  printf("alpha = %d\n", current.alpha );
  printf("wfc_contect_width = %d\n", current.wfc_context_width );
  printf("wfc_context_height = %d\n", current.wfc_context_height );

  printf("*********************************************\n\n");

}

void printState(OMX_HANDLETYPE handle) {
  OMX_STATETYPE state;
  OMX_ERRORTYPE err;

  err = OMX_GetState(handle, &state);
  if (err != OMX_ErrorNone) {
    fprintf(stderr, "Error on getting state\n");
    exit(1);
  }
  switch (state) {
  case OMX_StateLoaded:           printf("StateLoaded\n"); break;
  case OMX_StateIdle:             printf("StateIdle\n"); break;
  case OMX_StateExecuting:        printf("StateExecuting\n"); break;
  case OMX_StatePause:            printf("StatePause\n"); break;
  case OMX_StateWaitForResources: printf("StateWait\n"); break;
  case OMX_StateInvalid:          printf("StateInvalid\n"); break;
  default:                        printf("State unknown\n"); break;
  }
}


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}
