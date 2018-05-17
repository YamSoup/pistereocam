// this api is for a one button interface with software debouncing on that button
// could "easily" be adapted for multiple buttons but I have no need

#ifndef _ONEBUT_H
#define _ONEBUT_H

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

//for clarity
#define button_up false
#define button_down true

//modify these values (should be explict from names)
#define EXITCOUNTLIMIT 8
#define PIN_NUM 0

//struct to keep tabs on buttons state
struct buttonControl
{
  pthread_mutex_t mutexPtr;
  bool buttonState;
  bool takePhoto;
  int exitCount;
  bool exitCountReached;
} buttonControl;
  //remember to set sruct values to 0

void *myButtonPoll(void *voidButtonControl);


#endif // _ONEBUT_H
