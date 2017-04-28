// an  example of an button using an interupt, perfect for the camera

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define PIN_NUM 0

int count = 0;
uint16_t state = 0;
int interuptInProgress = false;

void myInterrupt()
{
  //NEED to check if mutex is locked and abort
  //this is not doing the job
  if (interuptInProgress == true) return;

  //this chunk of code is working quite well
  piLock(interuptInProgress);
  state = 0;
  int x;
  for(x = 0; x < 50; x++) {
    state = state | (uint16_t)digitalRead(PIN_NUM);
    if (state == 0xffff) {
      printf("Debounced Button Press\n");
      break;
    }
    state = state << 1;    
  }
  piUnlock(interuptInProgress);
  return;			    
}

int main (void)
{
  int result = 0;
  result = wiringPiSetup();
  printf("result = %d\n", result);

  pinMode(PIN_NUM, INPUT);
  pullUpDnControl(PIN_NUM, PUD_UP);
  
  wiringPiISR(PIN_NUM, INT_EDGE_FALLING, &myInterrupt); 

  while(1)
    {
      usleep(400);
    }
  
  return 0;
}

