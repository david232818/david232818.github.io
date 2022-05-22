#ifndef __HACK_NETWORK_H__
#define __HACK_NETWORK_H__

#define SUCCESS 1
#define FAILURE 0

int sendstr(int sockfd, unsigned char *buffer);

int recvline(int sockfd, unsigned char *dest_buffer);

#endif
