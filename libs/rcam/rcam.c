//place holder for remote cam helper funstions

#include "rcam.h"
#include "bcm_host.h"
#include "ilclient.h"

/*
what do I want from this

I want server functions to create the preview renderer for the remote camera INITIALIZE
I want to to be able to start and stop the preview from the server

next: I want to be able to control settings for the remote camera from the server
next: I want to be able to take a photo and have it sent to the server

I want to destroy the initlaization

maybe client side functions to simplify/
 */

//test funstion to check i can raise ilclient objexts from a linked file
int testFunction(ILCLIENT_T *client)
{
  COMPONENT_T *cameraTest;
  int result;
  
  result = ilclient_create_component(client,
                            &cameraTest,
                            "camera",
                            ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_OUTPUT_BUFFERS);
  printState(ilclient_get_handle(cameraTest));

  return result;
}

/*
This function creates a renderer on the server and comunicates with rcam client program to
display a preview until stopped 
Somehow needs to take a photo as well?
not really sure how to control once started?
unsure if this is possible - possibly by creating a global context to control the loop??

as a function that needs to be a thread this can only have 1 argument
and so will need a pointer to a struct containing all the needed parameters.
the struct should go into a header.

also a "global variable" will be needed to control the camera and mutex to control the the variable from both sides
either that or possibly a semephore to control the state

The display type selects if the preview runs full screen. or on the left or right side of a side by side.
*/
int serverStartRcam(ILCLIENT_T *client, int previewWidth, int previewHeight, int displayType);
