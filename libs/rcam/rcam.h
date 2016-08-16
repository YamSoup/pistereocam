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

/////////////////////////////////////////////////////////////////
// Function Prototypes

int testFunction(ILCLIENT_T *client);

#endif // _RCAM_H
