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
int getAndBindTCPServerSocket(char* port)
{
  int socket_fd;
  struct addrinfo hints, *results;
  int socket_status = 0;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // getaddrinfo
  socket_status = getaddrinfo(NULL, port, &hints, &results);
  if(socket_status != 0)
    {
      fprintf(stderr, "getaddrinfo failed, error = %s\n", gai_strerror(socket_status));
      exit(EXIT_FAILURE);
    }

  //socket
  socket_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(socket_fd <= 0)
    {
      fprintf(stderr, "socket call failed\n");
      exit(EXIT_FAILURE);
    }

  // bind
  socket_status = bind(socket_fd, results->ai_addr, results->ai_addrlen);
  if(socket_status != 0)
    {
      fprintf(stderr, "bind call failed, error = %s\n", gai_strerror(socket_status));
      exit(EXIT_FAILURE);
    }

  freeaddrinfo(results);

  return socket_fd;
}

int listenAndAcceptTCPServerSocket(int socket_to_listen_on, int backlog)
{
  int accepted_socket = 0;
  struct sockaddr_storage client_addr;
  socklen_t addr_size = sizeof(client_addr);
  int socket_status = 0;
  
  //listen
  socket_status = listen(socket_to_listen_on, 10/*backlog*/);
  if (socket_status == -1)
    {
      fprintf(stderr, "error on socket listen");
      exit(EXIT_FAILURE);
    }
  
  //accept
  accepted_socket = accept(socket_to_listen_on, (struct sockaddr *) &client_addr, &addr_size);
  if(accepted_socket <= 0)
    {
      fprintf(stderr, "error accepting socket");
      exit(EXIT_FAILURE);
    }

  //return the new socket
  return accepted_socket;
}

//returns a bound socket for client
int getAndConnectTCPSocket(char* address, char* port)
{
  int socket_fd;
  struct addrinfo hints, *servinfo, *p;
  int return_value = 0;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

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
