#include "http_server.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

/* sendstr: send data in buf to sockfd */
int sendstr(int sockfd, unsigned char *buf)
{
    int sent_bytes, bytes_to_send;

    bytes_to_send = strlen(buf);
    while (bytes_to_send > 0) {
	sent_bytes = send(sockfd, buf, bytes_to_send, 0);
	if (sent_bytes == -1)
	    return FAILURE;
	bytes_to_send -= sent_bytes;
	buf += sent_bytes;
    }
    return SUCCESS;
}
