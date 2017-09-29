// COMPILE WITH
// gcc -o button button.c -g --DRASPBEY_PI -DOMX_SKIP64BIT -pthread -lwiringPi
// needs pthread and lwiringPi

// an  example of an button using an interupt, perfect for the camera
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define PIN_NUM 0
#define button_up false
#define button_down true
#define EXITCOUNTLIMIT 10

  //a struct for button data to be used in main loop of camera eventually
  struct buttonControl
  {
    pthread_mutex_t mutexPtr;
    bool buttonState;
    bool takePhoto;
    int exitCount;
    bool exitCountReached;
  } buttonControl;
  //remember to set sruct values to 0


void *myButtonPoll(void *voidButtonControl)
{
  struct buttonControl *buttonControl = voidButtonControl;
  uint32_t state = 0x0e0e; //set as neither all 1's or 0's

  while(1) {
    pthread_mutex_lock(&buttonControl->mutexPtr);
    //printf("state = ox%x\n", state);
    state = state | (uint32_t)digitalRead(PIN_NUM);
    
    if (state == 0x00000000)
      {
	if (buttonControl->buttonState == button_up)
	  {
	    //printf("Debounced Button Press\n");
	    buttonControl->buttonState = button_down;
	    buttonControl->takePhoto = true;
	  }
	buttonControl->exitCount = buttonControl->exitCount + 1;
	printf("exit count = %d\n", buttonControl->exitCount);
	usleep(200000);
      }
    if (state == 0xffffffff) {
      if (buttonControl->buttonState == button_down)
	{
	  //printf("Debounced Button release\n");
	  buttonControl->buttonState = button_up;
	  buttonControl->exitCount = 0;
	}
      //else
      
      usleep(200000);
    }
    if (buttonControl->exitCount > EXITCOUNTLIMIT)
      buttonControl->exitCountReached = true;
    
    state = state << 1; //bitshift state 1 (VERY important)

    pthread_mutex_unlock(&buttonControl->mutexPtr);
    //usleep needed (belive the debounce is so bad multiple detections still happen)
    //this usleep happens between the bitshift operator
    usleep(200); 
  }
} 


//use a mutex to control the camera
//with a count that goes up every down button detected but is reset when the button is lifted
//to take pictures and if over a value to close the program
// dont forget to lock mutex in main loop when checking the value
// and in the button thread when changing value

int main (void)
{
  int result = 0;
  pthread_t button_id;

  memset(&buttonControl, 0, sizeof buttonControl);
  pthread_mutex_init(&buttonControl.mutexPtr, NULL);
  
  result = wiringPiSetup();
  printf("WiringPi result = %d\n", result);  
  
  pinMode(PIN_NUM, INPUT);
  pullUpDnControl(PIN_NUM, PUD_UP);

  // DO NOT USE
  // wiringPiISR(PIN_NUM, INT_EDGE_BOTH, &myInterrupt); 
  // the 'ISR' WiringPi solution boils down to a high prioty pthread with a for loop
  // leaving this warning in as a reminder not to use

  result = pthread_create(&button_id, NULL, myButtonPoll, &buttonControl);
  printf("thread result = %d\n", result);
  
  printf("pre main loop\n\n");
  while(1)
    {
      pthread_mutex_unlock(&buttonControl.mutexPtr);
      if (buttonControl.takePhoto == true)
	{
	  printf("take photo\n");
	  buttonControl.takePhoto = false;
	}
      if (buttonControl.exitCountReached == true)
	{
	  printf("exiting\n"); break;
	}
      
      
      //do stuff
      usleep(400);
      pthread_mutex_lock(&buttonControl.mutexPtr);
    }
  
  return 0;
}

