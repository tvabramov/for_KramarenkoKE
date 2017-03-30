#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "common.h"

int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sockfd == -1) {
		perror("[UDP CLIENT]: socket");
		goto error;
	}

	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &srv_addr.sin_addr) == 0) {
		perror("[UDP CLIENT]: inet_aton");
		goto error;
	}

{
	socklen_t srv_addr_len = sizeof(srv_addr);
	char buf[BUFLEN + 1];
	sprintf(buf, "Hello, i am client");
	if (sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&srv_addr, srv_addr_len) == -1) {
		perror("[UDP CLIENT]: sendto");
		goto error;
	}

	printf("[UDP CLIENT]: Sended message \"%s\"\n", buf);
}
	shutdown(sockfd, SHUT_RDWR);
	exit(EXIT_SUCCESS);

error:
	shutdown(sockfd, SHUT_RDWR);

        exit(EXIT_FAILURE);
}
