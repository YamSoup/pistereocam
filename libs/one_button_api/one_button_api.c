// check below is correct
#include "one_button_api.h"


//only one function but pretty powerful
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
      {buttonControl->exitCountReached = true;
	break;}
    
    state = state << 1; //bitshift state 1 (VERY important)

    pthread_mutex_unlock(&buttonControl->mutexPtr);
    //usleep needed (belive the debounce is so bad multiple detections still happen)
    //this usleep happens between the bitshift operator
    usleep(400); 
  }
    //unlock mutex before teminating
  pthread_mutex_unlock(&buttonControl->mutexPtr);
    
  //call pthread_exit so caller can join
  pthread_exit(NULL);
}

/* USE EXAMPLE
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
      pthread_mutex_lock(&buttonControl.mutexPtr);
      usleep(400);
      
    }
  
  return 0;
}

*/
