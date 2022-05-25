#include "http_server.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#define EOL "\r\n"
#define EOL_SIZE 2

#define NOEOL 0

/*
 * recvline: receive data from sockfd until \r\n(EOL) and return
 * received bytes exclude \r\n(EOL)
 */
int recvline(int sockfd, unsigned char *dest_buf)
{
    unsigned char *dest_bufp;
    int eol_matched;

    eol_matched = 0;
    for (dest_bufp = dest_buf;
	 recv(sockfd, dest_bufp, 1, 0) == 1;
	 dest_bufp++) {
	if (*dest_bufp == EOL[eol_matched]) {
	    eol_matched++;
	    if (eol_matched == EOL_SIZE) {
		*(dest_bufp - EOL_SIZE) = '\0';
		return strlen(dest_buf);
	    }
	} else
	    eol_matched = 0;
    }
    return NOEOL;
}
