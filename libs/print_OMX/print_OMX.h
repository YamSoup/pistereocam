/*******************************************************************************
A Library that prints some of the most common omx structures or states
with a focus on video and structures that cause runtime errors.

Parts are from Jan Newmarch if you havent seen the site yet
https://jan.newmarch.name/RPi/index.html
great resource.

TODO:
Native window/Render/device type is a void * possibly show as an address

*******************************************************************************/

#ifndef _PRINT_OMX_H
#define _PRINT_OMX_H

void printState(OMX_HANDLETYPE handle);
void printBits(void *toPrint);
void print_OMX_CONFIG_DISPLAYREGIONTYPE(OMX_CONFIG_DISPLAYREGIONTYPE current);
char *err2str(int err);

//**************************************************************
// Below this line are the structures needed for Port stuff
//**************************************************************

void print_OMX_AUDIO_CODINGTYPE(OMX_AUDIO_CODINGTYPE eEncoding);
void print_OMX_VIDEO_CODINGTYPE(OMX_VIDEO_CODINGTYPE eCompressionFormat);
void print_OMX_IMAGE_CODINGTYPE(OMX_IMAGE_CODINGTYPE eCompressionFormat);
void print_OMX_COLOR_FORMATTYPE(OMX_COLOR_FORMATTYPE eColorFormat);

void print_OMX_OTHER_PORTDEFINITIONTYPE(OMX_OTHER_PORTDEFINITIONTYPE other);
void print_OMX_AUDIO_PORTDEFINITIONTYPE(OMX_AUDIO_PORTDEFINITIONTYPE audio);
void print_OMX_VIDEO_PORTDEFINITIONTYPE(OMX_VIDEO_PORTDEFINITIONTYPE video);
void print_OMX_IMAGE_PORTDEFINITIONTYPE(OMX_IMAGE_PORTDEFINITIONTYPE image);

void print_OMX_PARAM_PORTDEFINITIONTYPE(OMX_PARAM_PORTDEFINITIONTYPE params);
void print_OMX_IMAGE_PARAM_PORTFORMATTYPE(OMX_IMAGE_PARAM_PORTFORMATTYPE image_params);

//**************************************************************
// Stuff from Raspberry Pi GPU Audio Video Programming, Jan Newmarch
//**************************************************************

//prints available components (not used components)
void print_OMX_component_list();
//this might be the same as print_OMX_PARAM_PORTDEFINITIONTYPE
void print_OMX_port_information(OMX_PARAM_PORTDEFINITIONTYPE *portdef);

#endif /* _PRINT_OMX_H */
