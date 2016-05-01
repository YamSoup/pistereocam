/*******************************************************************************
Created By Stephen Ford April 2016

A Collection of functions that prints some of the most common omx structures or states
with a focus on video and structures that cause runtime errors.

Parts are from Jan Newmarch if you havent seen the site yet
https://jan.newmarch.name/RPi/index.html
great resource.

TODO:
Native window/Render/device type is a void * possibly show as an address

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_logging.h"
#include "interface/vmcs_host/vchost.h"

#include "IL/OMX_Broadcom.h"
#include "print_OMX.h"

char *err2str(int err) {
  switch (err)
  {
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
  printf("-----------------------\n");
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

  printf("-----------------------\n");

}

//**************************************************************
// Below this line are the structures needed for Port stuff
//**************************************************************

void print_OMX_AUDIO_CODINGTYPE(OMX_AUDIO_CODINGTYPE eEncoding)
{
    switch(eEncoding)
    {
    case OMX_AUDIO_CodingUnused:        printf("OMX_AUDIO_CodingUnused - Placeholder value when coding is N/A"); break;
    case OMX_AUDIO_CodingAutoDetect:    printf("OMX_AUDIO_CodingAutoDetect - auto detection of audio format"); break;
    case OMX_AUDIO_CodingPCM:           printf("OMX_AUDIO_CodingPCM - Any variant of PCM coding"); break;
    case OMX_AUDIO_CodingADPCM: 	    printf("OMX_AUDIO_CodingADPCM - Any variant of ADPCM encoded data"); break;
    case OMX_AUDIO_CodingAMR: 	        printf("OMX_AUDIO_CodingAMR - Any variant of AMR encoded data"); break;
    case OMX_AUDIO_CodingGSMFR: 	    printf("OMX_AUDIO_CodingGSMFR - Any variant of GSM fullrate (i.e. GSM610)"); break;
    case OMX_AUDIO_CodingGSMEFR: 	    printf("OMX_AUDIO_CodingGSMEFR - Any variant of GSM Enhanced Fullrate encoded data"); break;
    case OMX_AUDIO_CodingGSMHR: 	    printf("OMX_AUDIO_CodingGSMHR - Any variant of GSM Halfrate encoded data"); break;
    case OMX_AUDIO_CodingPDCFR: 	    printf("OMX_AUDIO_CodingPDCFR - Any variant of PDC Fullrate encoded data"); break;
    case OMX_AUDIO_CodingPDCEFR: 	    printf("OMX_AUDIO_CodingPDCEFR - Any variant of PDC Enhanced Fullrate encoded data"); break;
    case OMX_AUDIO_CodingPDCHR: 	    printf("OMX_AUDIO_CodingPDCHR - Any variant of PDC Halfrate encoded data"); break;
    case OMX_AUDIO_CodingTDMAFR: 	    printf("OMX_AUDIO_CodingTDMAFR - Any variant of TDMA Fullrate encoded data (TIA/EIA-136-420)"); break;
    case OMX_AUDIO_CodingTDMAEFR: 	    printf("OMX_AUDIO_CodingTDMAEFR - Any variant of TDMA Enhanced Fullrate encoded data (TIA/EIA-136-410)"); break;
    case OMX_AUDIO_CodingQCELP8: 	    printf("OMX_AUDIO_CodingQCELP8 - Any variant of QCELP 8kbps encoded data"); break;
    case OMX_AUDIO_CodingQCELP13: 	    printf("OMX_AUDIO_CodingQCELP13 - Any variant of QCELP 13kbps encoded data"); break;
    case OMX_AUDIO_CodingEVRC: 	        printf("OMX_AUDIO_CodingEVRC - Any variant of EVRC encoded data"); break;
    case OMX_AUDIO_CodingSMV: 	        printf("OMX_AUDIO_CodingSMV - Any variant of SMV encoded data"); break;
    case OMX_AUDIO_CodingG711: 	        printf("OMX_AUDIO_CodingG711 - Any variant of G.711 encoded data"); break;
    case OMX_AUDIO_CodingG723: 	        printf("OMX_AUDIO_CodingG723 - Any variant of G.723 dot 1 encoded data"); break;
    case OMX_AUDIO_CodingG726: 	        printf("OMX_AUDIO_CodingG726 - Any variant of G.726 encoded data"); break;
    case OMX_AUDIO_CodingG729: 	        printf("OMX_AUDIO_CodingG729 - Any variant of G.729 encoded data"); break;
    case OMX_AUDIO_CodingAAC: 	        printf("OMX_AUDIO_CodingAAC - Any variant of AAC encoded data"); break;
    case OMX_AUDIO_CodingMP3: 	        printf("OMX_AUDIO_CodingMP3 - Any variant of MP3 encoded data"); break;
    case OMX_AUDIO_CodingSBC: 	        printf("OMX_AUDIO_CodingSBC - Any variant of SBC encoded data"); break;
    case OMX_AUDIO_CodingVORBIS: 	    printf("OMX_AUDIO_CodingVORBIS - Any variant of VORBIS encoded data"); break;
    case OMX_AUDIO_CodingWMA: 	        printf("OMX_AUDIO_CodingWMA - Any variant of WMA encoded data"); break;
    case OMX_AUDIO_CodingRA: 	        printf("OMX_AUDIO_CodingRA - Any variant of RA encoded data"); break;
    case OMX_AUDIO_CodingMIDI:      	printf("OMX_AUDIO_CodingMIDI - Any variant of MIDI encoded data"); break;
    case OMX_AUDIO_CodingMax:           printf("OMX_AUDIO_CodingMax"); break;
    default:                            printf("Format not accounted for = %d", eEncoding); break;
    }
    putchar('\n');
}

void print_OMX_VIDEO_CODINGTYPE(OMX_VIDEO_CODINGTYPE eCompressionFormat)
{
    switch(eCompressionFormat)
    {
    case OMX_VIDEO_CodingUnused:     printf("OMX_VIDEO_CodingUnused - 	Value when coding is N/A"); break;
    case OMX_VIDEO_CodingAutoDetect: printf("OMX_VIDEO_CodingAutoDetect - Autodetection of coding type"); break;
    case OMX_VIDEO_CodingMPEG2:      printf("OMX_VIDEO_CodingMPEG2 - AKA: H.262"); break;
    case OMX_VIDEO_CodingH263:       printf("OMX_VIDEO_CodingH263 - H.263"); break;
    case OMX_VIDEO_CodingMPEG4:      printf("OMX_VIDEO_CodingMPEG4 - MPEG-4"); break;
    case OMX_VIDEO_CodingWMV:        printf("OMX_VIDEO_CodingWMV - all versions of Windows Media Video"); break;
    case OMX_VIDEO_CodingRV:         printf("OMX_VIDEO_CodingRV - all versions of Real Video"); break;
    case OMX_VIDEO_CodingAVC:        printf("OMX_VIDEO_CodingAVC - H.264/AVC"); break;
    case OMX_VIDEO_CodingMJPEG:      printf("OMX_VIDEO_CodingMJPEG - Motion JPEG"); break;
    case OMX_VIDEO_CodingMax:        printf("OMX_VIDEO_CodingMax (not a video format)"); break;
    default:                         printf("Format not accounted for"); break;
    }
    putchar('\n');
}

void print_OMX_IMAGE_CODINGTYPE(OMX_IMAGE_CODINGTYPE eCompressionFormat)
{
    switch(eCompressionFormat)
    {
    case OMX_IMAGE_CodingUnused:        printf("OMX_IMAGE_CodingUnused - Value when format is N/A"); break;
    case OMX_IMAGE_CodingAutoDetect:    printf("OMX_IMAGE_CodingAutoDetect - Auto detection of image format"); break;
    case OMX_IMAGE_CodingJPEG:          printf("OMX_IMAGE_CodingJPEG - JPEG/JFIF image format"); break;
    case OMX_IMAGE_CodingJPEG2K:        printf("OMX_IMAGE_CodingJPEG2K - JPEG 2000 image format"); break;
    case OMX_IMAGE_CodingEXIF:          printf("OMX_IMAGE_CodingEXIF - EXIF image format"); break;
    case OMX_IMAGE_CodingTIFF:          printf("OMX_IMAGE_CodingTIFF - TIFF image format"); break;
    case OMX_IMAGE_CodingGIF:           printf("OMX_IMAGE_CodingGIF - Graphics image format"); break;
    case OMX_IMAGE_CodingPNG:           printf("OMX_IMAGE_CodingPNG - PNG image format"); break;
    case OMX_IMAGE_CodingLZW:           printf("OMX_IMAGE_CodingLZW - LZW image format"); break;
    case OMX_IMAGE_CodingBMP:           printf("OMX_IMAGE_CodingBMP - Windows Bitmap format"); break;
    case OMX_IMAGE_CodingMax:           printf("OMX_IMAGE_CodingMax (not an image format)"); break;
    default:                            printf("Format not accounted for"); break;
    }
    putchar('\n');
}

void print_OMX_COLOR_FORMATTYPE(OMX_COLOR_FORMATTYPE eColorFormat)
{
    switch(eColorFormat)
    {
    case OMX_COLOR_FormatUnused:                 printf("OMX_COLOR_FormatUnused"); break;
    case OMX_COLOR_FormatMonochrome:             printf("OMX_COLOR_FormatMonochrome"); break;
    case OMX_COLOR_Format8bitRGB332:             printf("OMX_COLOR_Format8bitRGB332"); break;
    case OMX_COLOR_Format12bitRGB444:            printf("OMX_COLOR_Format12bitRGB444"); break;
    case OMX_COLOR_Format16bitARGB4444:          printf("OMX_COLOR_Format16bitARGB4444"); break;
    case OMX_COLOR_Format16bitARGB1555:          printf("OMX_COLOR_Format16bitARGB1555"); break;
    case OMX_COLOR_Format16bitRGB565:            printf("OMX_COLOR_Format16bitRGB565"); break;
    case OMX_COLOR_Format16bitBGR565:            printf("OMX_COLOR_Format16bitBGR565"); break;
    case OMX_COLOR_Format18bitRGB666:            printf("OMX_COLOR_Format18bitRGB666"); break;
    case OMX_COLOR_Format18bitARGB1665:          printf("OMX_COLOR_Format18bitARGB1665"); break;
    case OMX_COLOR_Format19bitARGB1666:          printf("OMX_COLOR_Format19bitARGB1666"); break;
    case OMX_COLOR_Format24bitRGB888:            printf("OMX_COLOR_Format24bitRGB888"); break;
    case OMX_COLOR_Format24bitBGR888:            printf("OMX_COLOR_Format24bitBGR888"); break;
    case OMX_COLOR_Format24bitARGB1887:          printf("OMX_COLOR_Format24bitARGB1887"); break;
    case OMX_COLOR_Format25bitARGB1888:          printf("OMX_COLOR_Format25bitARGB1888"); break;
    case OMX_COLOR_Format32bitBGRA8888:          printf("OMX_COLOR_Format32bitBGRA8888"); break;
    case OMX_COLOR_Format32bitARGB8888:          printf("OMX_COLOR_Format32bitARGB8888"); break;
    case OMX_COLOR_FormatYUV411Planar:           printf("OMX_COLOR_FormatYUV411Planar"); break;
    case OMX_COLOR_FormatYUV411PackedPlanar:     printf("OMX_COLOR_FormatYUV411PackedPlanar"); break;
    case OMX_COLOR_FormatYUV420Planar:           printf("OMX_COLOR_FormatYUV420Planar"); break;
    case OMX_COLOR_FormatYUV420PackedPlanar:     printf("OMX_COLOR_FormatYUV420PackedPlanar"); break;
    case OMX_COLOR_FormatYUV420SemiPlanar:       printf("OMX_COLOR_FormatYUV420SemiPlanar"); break;
    case OMX_COLOR_FormatYUV422Planar:           printf("OMX_COLOR_FormatYUV422Planar"); break;
    case OMX_COLOR_FormatYUV422PackedPlanar:     printf("OMX_COLOR_FormatYUV422PackedPlanar"); break;
    case OMX_COLOR_FormatYUV422SemiPlanar:       printf("OMX_COLOR_FormatYUV422SemiPlanar"); break;
    case OMX_COLOR_FormatYCbYCr:                 printf("OMX_COLOR_FormatYCbYCr"); break;
    case OMX_COLOR_FormatYCrYCb:                 printf("OMX_COLOR_FormatYCrYCb"); break;
    case OMX_COLOR_FormatCbYCrY:                 printf("OMX_COLOR_FormatCbYCrY"); break;
    case OMX_COLOR_FormatCrYCbY:                 printf("OMX_COLOR_FormatCrYCbY"); break;
    case OMX_COLOR_FormatYUV444Interleaved:      printf("OMX_COLOR_FormatYUV444Interleaved"); break;
    case OMX_COLOR_FormatRawBayer8bit:           printf("OMX_COLOR_FormatRawBayer8bit"); break;
    case OMX_COLOR_FormatRawBayer10bit:          printf("OMX_COLOR_FormatRawBayer10bit"); break;
    case OMX_COLOR_FormatRawBayer8bitcompressed: printf("OMX_COLOR_FormatRawBayer8bitcompressed"); break;
    case OMX_COLOR_FormatL2:                     printf("OMX_COLOR_FormatL2"); break;
    case OMX_COLOR_FormatL4:                     printf("OMX_COLOR_FormatL4"); break;
    case OMX_COLOR_FormatL8:                     printf("OMX_COLOR_FormatL8"); break;
    case OMX_COLOR_FormatL16:                    printf("OMX_COLOR_FormatL16"); break;
    case OMX_COLOR_FormatL24:                    printf("OMX_COLOR_FormatL24"); break;
    case OMX_COLOR_FormatL32:                    printf("OMX_COLOR_FormatL32"); break;
    case OMX_COLOR_FormatYUV420PackedSemiPlanar: printf("OMX_COLOR_FormatYUV420PackedSemiPlanar"); break;
    case OMX_COLOR_FormatYUV422PackedSemiPlanar: printf("OMX_COLOR_FormatYUV422PackedSemiPlanar"); break;
    case OMX_COLOR_Format18BitBGR666:            printf("OMX_COLOR_Format18BitBGR666"); break;
    case OMX_COLOR_Format24BitARGB6666:          printf("OMX_COLOR_Format24BitARGB6666"); break;
    case OMX_COLOR_Format24BitABGR6666:          printf("OMX_COLOR_Format24BitABGR6666"); break;
    case OMX_COLOR_FormatMax:                    printf("OMX_COLOR_FormatMax (not a format)"); break;
    default:                                     printf("Format not accounted for"); break;
    }
    putchar('\n');
}

void print_OMX_OTHER_PORTDEFINITIONTYPE(OMX_OTHER_PORTDEFINITIONTYPE other)
{
  //OMX_OTHER_PORTDEFINITIONTYPE is a stucture with just one member eFormat of type OMX_OTHER_FORMATTYPE
  switch(other.eFormat)
    {
    case OMX_OTHER_FormatTime:              printf("OMX_OTHER_FormatTime - Transmission of various timestamps, elapsed time, time deltas, etc"); break;
    case OMX_OTHER_FormatPower:             printf("OMX_OTHER_FormatPower - Perhaps used for enabling/disabling power management, setting clocks?"); break;
    case OMX_OTHER_FormatStats:             printf("OMX_OTHER_FormatStats - Could be things such as frame rate, frames dropped, etc"); break;
    case OMX_OTHER_FormatBinary:            printf("OMX_OTHER_FormatBinary - Arbitrary binary data"); break;
    case OMX_OTHER_FormatVendorReserved:    printf("OMX_OTHER_FormatVendorReserved - Starting value for vendor specific formats"); break;
    case OMX_OTHER_FormatMax:               printf("OMX_OTHER_FormatMax (not a format)"); break;
    default:                                printf("Format not accounted for"); break;
    }
    putchar('\n');
}

void print_OMX_AUDIO_PORTDEFINITIONTYPE(OMX_AUDIO_PORTDEFINITIONTYPE audio)
{
    printf("   audio.cMIMEType = %s\n", audio.cMIMEType);
    printf("   audio.pNativeRender = %d\n", (int)audio.pNativeRender);
    printf("   audio.bFlagErrorConcealment = ");
    audio.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
    printf("   audio.eEncoding = ");
    print_OMX_AUDIO_CODINGTYPE(audio.eEncoding);
}

void print_OMX_VIDEO_PORTDEFINITIONTYPE(OMX_VIDEO_PORTDEFINITIONTYPE video)
{
    printf("OMX_VIDEO_PORT_DEFINITIONTYPE\n");
    printf("   video.cMIMEType = %s\n", video.cMIMEType);
    printf("   video.pNativeRender = %d \n", (int)video.pNativeRender);
    printf("   video.nFrameWidth = %d\n", (int)video.nFrameWidth);
    printf("   video.nFrameHeight = %d\n", (int)video.nFrameHeight);
    printf("   video.nStride = %d\n", (int)video.nStride);
    printf("   video.nSliceHeight = %d\n", (int)video.nSliceHeight);
    printf("   video.nBitrate = %d\n", (int)video.nBitrate);
    printf("   video.xFramerate >> 16 = %d\n", (int)video.xFramerate >>16 );
    printf("   video.bFlagErrorConcealment = ");
    video.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
    printf("   video.eCompressionFormat = ");
    print_OMX_COLOR_FORMATTYPE(video.eCompressionFormat);
    printf("   video.eColorFormat = ");
    print_OMX_COLOR_FORMATTYPE(video.eColorFormat);
    printf("   video.pNativeWindow = %d\n", (int)video.pNativeWindow);
}

void print_OMX_IMAGE_PORTDEFINITIONTYPE(OMX_IMAGE_PORTDEFINITIONTYPE image)
{
    printf("   image.cMIMEType = %s\n", image.cMIMEType);
    printf("   image.pNativeRender = %d\n", (int)image.pNativeRender);
    printf("   image.nFrameWidth = %d\n", (int)image.nFrameWidth);
    printf("   image.nFrameHeight = %d\n", (int)image.nFrameHeight);
    printf("   image.nStride = %d\n", (int)image.nStride);
    printf("   image.nSliceHeight = %d\n", (int)image.nSliceHeight);
    printf("   image.bFlagErrorConcealment = ");
    image.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
    printf("   image.eCompressionFormat = ");
    print_OMX_IMAGE_CODINGTYPE(image.eCompressionFormat);
    printf("   image.eColorFormat = ");
    print_OMX_COLOR_FORMATTYPE(image.eColorFormat);
    printf("   image.pNativeWindow = %d\n", (int)image.pNativeWindow);
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
