// an  example of an button using an interupt, perfect for the camera

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define PIN_NUM 0

int count = 0;
uint8_t state = 0;
int interuptInProgress = false;

void myInterrupt()
{
  //check to see if already processing an inturrpt
  //if (interuptInProgress == true) return;

  //change this to an actual mutex
  piLock(interuptInProgress);
  printf("In inturupt\n");
  int x;
  for(x = 0; x < 5000; x++) {
    state = state | (uint8_t)digitalRead(PIN_NUM);
    state = state << 1;
    if (state == 0xffff) {
      printf("Debounced Button Press\n");
      break;
    }    
  }
  printf("Exit without debounce");
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

