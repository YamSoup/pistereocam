// an example of an button using an interupt, perfect for the camera

#include <wiringPi.h>
#include <stdio.h>

#define CAM_PIN 4

void myInterrupt(void)
{
  printf("button change");
}

int main (void)
{
  
  //investigate this first (as does not require sudo but does require the gpios to be set somehow beforehand)
  //looking online this might be a poor choise but investigate further
  int result = 0;
  result = wiringPiSetupSys();
  printf("result = %d", result);

  
  wiringPiISR(CAM_PIN, INT_EDGE_BOTH, &myInterrupt); 

  for(;;)
    {
      //loop
    }
  
  return 0;
}

