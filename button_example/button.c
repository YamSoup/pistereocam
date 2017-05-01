// an  example of an button using an interupt, perfect for the camera
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define PIN_NUM 0
#define button_up false
#define button_down true

int count = 0;

void *myButtonPoll()
{
  bool button_position = button_up;
  uint16_t state = 0x0e0e;
  while(1) {
    //printf("state = ox%x\n", state);
    state = state | (uint16_t)digitalRead(PIN_NUM);
    if (state == 0x0000 && button_position == button_up) {
      printf("Debounced Button Press\n");
      button_position = button_down;
    }
    if (state == 0xffff && button_position == button_down) {
      printf("Debounced Button release\n");
      button_position = button_up;
    }
    state = state << 1;
    //usleep needed (belive the debounce is so bad multiple detections still happen)
    usleep(10); 
  }
} 
  

int main (void)
{
  int result = 0;
  pthread_t button_id;
  
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
  
  printf("pre main loop\n\n");
  while(1)
    {
      usleep(400);
    }
  
  return 0;
}

