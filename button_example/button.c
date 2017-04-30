// an  example of an button using an interupt, perfect for the camera
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define PIN_NUM 0

int count = 0;
uint16_t state = 0;
pthread_mutex_t interupt_mutex;

void *myButtonPoll()
{
  //needs a lot of work! but getting there
  int x;
  while(1) {
  if (pthread_mutex_trylock(&interupt_mutex) == 0) {
    //this chunk of code is working quite well
    state = 0;
    for(x = 0; x < 20; x++) {
      //printf("state = ox%x\n", state);
      state = state | (uint16_t)digitalRead(PIN_NUM);
      if (state == 0x0000) {
	//might need to test for button up with 0xffff
	printf("Debounced Button Press\n");
	sleep(1);
	break;
      }
      state = state << 1;    
    }
  pthread_mutex_unlock(&interupt_mutex);
  }
  else printf("trylock block\n");  
  }
}

int main (void)
{
  int result = 0;
  pthread_t button_id;
  
  result = pthread_mutex_init(&interupt_mutex, NULL);
  printf("Mutex initalised state = %d\n", result);
  result = wiringPiSetup();
  printf("WiringPi result = %d\n", result);  
  
  pinMode(PIN_NUM, INPUT);
  pullUpDnControl(PIN_NUM, PUD_UP);

  // DO NOT USE
  // wiringPiISR(PIN_NUM, INT_EDGE_BOTH, &myInterrupt); 
  // the 'ISR' WiringPi solution boils down to a high prioty pthread with a for loop
  // leaving this warning in as a reminder not to use

  result = pthread_create(&button_id, NULL, myButtonPoll, NULL);
  printf("thread result = %d\n", result);
  
  printf("pre main loop");
  while(1)
    {
      usleep(400);
    }
  
  return 0;
}

