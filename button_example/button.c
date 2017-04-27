// an example of an button using an interupt, perfect for the camera

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define PIN_NUM 0

int count = 0;
uint16_t state = 0;
bool inuptInProgress = false;

void myInterrupt()
{
  printf("In Inturupt");
  //check to see if already processing an inturrpt
  if (inuptInProgress == true) return;

  inuptInProgress = true;
  int i;
  for(i = 0; i > 1000; i++)
    {
      state = 0;
      state = (state << 1) | digitalRead(PIN_NUM) | 0xe000;
      
      if(state == 0xf000)
	{
	  printf("button down/n");
	}
      else
	printf("Button something");
    }
  inuptInProgress = false;
}

int main (void)
{
  
  //investigate this first (as does not require sudo but does require the gpios to be set somehow beforehand)
  //looking online this might be a poor choise but investigate further
  int result = 0;
  result = wiringPiSetup();
  printf("result = %d/n", result);

  pinMode(PIN_NUM, INPUT);
  pullUpDnControl(PIN_NUM, PUD_UP);
  
  wiringPiISR(PIN_NUM, INT_EDGE_FALLING, &myInterrupt); 

  for(;;)
    {
      usleep(10000);
      printf("0x%16x\n", state);
    }
  
  return 0;
}

