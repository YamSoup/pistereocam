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

void myInterrupt()
{
  //NEED to check if mutex is locked and abort
  //possibly change to pthread mutex so i can do:
  if (pthread_mutex_trylock(&interupt_mutex) == 0) {
    //this chunk of code is working quite well
    state = 0;
    int x;
    for(x = 0; x < 500; x++) {
      state = state | (uint16_t)digitalRead(PIN_NUM);
      if (state == 0xffff) {
	printf("Debounced Button Press\n");
	break;
      }
      state = state << 1;    
    }
    pthread_mutex_unlock(&interupt_mutex);
    return;
  }
  printf("trylock block\n");
  return;
  
}

int main (void)
{
  int result = 0;
  result = pthread_mutex_init(&interupt_mutex, NULL);
  printf("Mutex initalised state = %d\n", result);
  result = wiringPiSetup();
  printf("WiringPi result = %d\n", result);  
  
  pinMode(PIN_NUM, INPUT);
  pullUpDnControl(PIN_NUM, PUD_UP);
  
  wiringPiISR(PIN_NUM, INT_EDGE_FALLING, &myInterrupt); 
  printf("pre loop");
  while(1)
    {
      usleep(400);
    }
  
  return 0;
}

