#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "http_server.h"

#define	PORT	7890
#define WEBROOTDIR "../webroot"
#define BACKLOGQUEUE_MAX 20

void fatal(char *err);
void handle_conn(int, struct sockaddr_in *);

int main()
{
    int sockfd, new_sockfd, yes;
    struct sockaddr_in host_addr, client_addr;
    socklen_t sin_size;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	fatal("while making socket");

    yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
	== -1)
	fatal("while setting socket options");

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PORT);
    host_addr.sin_addr.s_addr = 0; /* fill with my address automatically */
    memset(&(host_addr.sin_zero), 0x00, 8);

    if (bind(sockfd, (struct sockaddr *) &host_addr,
	     sizeof(struct sockaddr)) == -1)
	fatal("while binding socket");

    if (listen(sockfd, 5) == -1)
	fatal("while listening to the socket");

    while (1) {
	sin_size = sizeof(struct sockaddr_in);
	new_sockfd = accept(sockfd, (struct sockaddr *) &client_addr,
			    &sin_size);
	if (new_sockfd == -1)
	    fatal("while accepting connection");

	handle_conn(new_sockfd, &client_addr);
    }
    return 0;
}

/* fatal: print error message */
void fatal(char *err)
{
    perror(err);
    exit(EXIT_FAILURE);
}

/* handle_conn: handle connection between host and client */
void handle_conn(int sockfd, struct sockaddr_in *client_addrp)
{
    unsigned char *requestp, request[512], resource[512];
    int length;
    FILE *rfp;

    length = recvline(sockfd, request);

    printf("Request [%s:%d] \"%s\"\n",
	   inet_ntoa(client_addrp->sin_addr),
	   ntohs(client_addrp->sin_port),
	   request);

    requestp = strstr(request, " HTTP/");
    if (requestp == NULL)
	printf(" NOT HTTP!\n");
    else {
	*requestp = 0;
	requestp = NULL;
	if (strncmp(request, "GET ", 4) == 0)
	    requestp = request + 4;
	if (strncmp(request, "HEAD ", 5) == 0)
	    requestp = request + 5;

	if (requestp == NULL)
	    printf("\tUnkown REQUEST!\n");
	else {
	    if (requestp[strlen(requestp) - 1] == '/')
		strcat(requestp, "index.html");
	    strcpy(resource, WEBROOTDIR);
	    strcat(resource, requestp);
	    if ((rfp = fopen(resource, "r")) == NULL) {
		printf(" 404 Not Found\n");
		sendstr(sockfd, "HTTP/1.1 404 NOT FOUND\r\n");
		sendstr(sockfd, "Server: Tiny webserver\r\n\r\n");
		sendstr(sockfd, "<html><head><title>404 Not Found</title>"
			"</head>");
		sendstr(sockfd, "<body><h1>URL Not Found</h1></body>"
			"</html>\r\n");
	    } else {
		printf(" 200 OK\r\n");
		sendstr(sockfd, "HTTP/1.1 200 OK\r\n");
		sendstr(sockfd, "Server: Tiny webserver\r\n\r\n");
		if (requestp == request + 4) {
		    if ((requestp = malloc(BUFSIZ)) == NULL)
			fatal("while allocating memory");
		    while (!feof(rfp)) {
			memset(requestp, 0x00, BUFSIZ);
			fgets(requestp, BUFSIZ, rfp);
			requestp[strlen(requestp) - 1] = '\0';
			send(sockfd, requestp, strlen(requestp), 0);
		    }
		    free(requestp);
		}
		fclose(rfp);
	    }
	}
    }
    shutdown(sockfd, SHUT_RDWR);
}
