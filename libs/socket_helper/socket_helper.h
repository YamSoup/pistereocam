/*******************************************************************************
Created By Stephen Ford April 2016

A Collection of Functions that simplies the use of sockets in c
This is pretty basic as I am new to this

*******************************************************************************/

#ifndef SOCKET_HELPER_H
#define SOCKET_HELPER_H

#define SOCKTYPE_TCP 1
#define SOCKTYPE_UDP 2

//returns a bound socket
int getAndConnectTCPSocket(char* address, char* port);

//write all and read all (to ensure full buffer is sent)
void write_all(int socket, const void *buf, size_t num_bytes);
void read_all(int socket, const void *buf, size_t num_bytes);

#endif /*SOCKET_HELPER_H*/
