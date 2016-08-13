/*******************************************************************************
Created By Stephen Ford April 2016

A Collection of Functions that simplies the use of sockets in c
This is pretty basic as I am new to this

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "socket_helper.h"

//TODO create 2 server side functions
// 1 to create a socket and bind
// 2 to listen and accept on created socket
// use current stereo_cam.c as template
// is this TCP only?
int getAndBindServerSocket(int socket_type, char* port);

int listenAndAcceptServerSocket(int socket_to_listen_on, int backlog);

//returns a bound socket
int getAndConnectSocket(int socket_type, char* address, char* port)
{
  int socket_fd;
  struct addrinfo hints, *servinfo, *p;
  int return_value = 0;

  printf("%d", socket_type);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  if(socket_type == SOCKTYPE_UDP)
    hints.ai_socktype = SOCK_DGRAM;
  else if(socket_type == SOCKTYPE_TCP)
    hints.ai_socktype = SOCK_STREAM;
  else
    {
      printf("unknown socket type\n");
      return -2;
    }

  //use getaddrinfo to populate servinfo
  return_value = getaddrinfo(address, port, &hints, &servinfo);
  if (return_value != 0)
    {
      fprintf(stderr, "Error using getaddrinfo, Error: %s\n", gai_strerror(return_value));
      exit(EXIT_FAILURE);
    }

  //loop through results to get a valid socket
  for(p = servinfo; p != NULL; p = p->ai_next)
    {
      //attempt get socket
      socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if(socket_fd == -1)
	{
	  fprintf(stderr, "Error using socket function\n");
	  continue;
	}
      //attempt connect on socket! (will bind socket to free port automatically)
      return_value = connect(socket_fd, p->ai_addr, p->ai_addrlen);
      if(return_value == -1)
	{
	  fprintf(stderr, "Error connecting to socket\n");
	  continue;
	}

      break;
    }

  if (p == NULL)
    {
      fprintf(stderr, "talker:failed to connect on socket\n");
      return -1;
    }

  freeaddrinfo(servinfo);
  return socket_fd;
}

//write all (to ensure full buffer is sent)

void write_all(int socket, const void *buf, size_t num_bytes)
{
  size_t current_writen = 0;
  while (current_writen < num_bytes)
    current_writen += write(socket, &buf + current_writen, num_bytes - current_writen);
}

void read_all(int socket, const void *buf, size_t num_bytes)
{
  size_t current_read = 0;
  while (current_read < num_bytes)
    current_read += read(socket, &buf + current_read, num_bytes - current_read);
}
