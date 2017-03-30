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
		perror("[UDP SERVER]: socket");
		goto error;
	}

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
		perror("[UDP SERVER]: bind");
                goto error;
	}

{
	struct sockaddr_in out_addr;
	socklen_t out_addr_len;
	char buf[BUFLEN + 1];
	if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&out_addr, &out_addr_len) == -1) {
		perror("[UDP SERVER]: recvfrom");
		goto error;
	}

	printf("[UDP SERVER]: Recieved message \"%s\"\n", buf);
}
	shutdown(sockfd, SHUT_RDWR);
	exit(EXIT_SUCCESS);

error:
	shutdown(sockfd, SHUT_RDWR);

        exit(EXIT_FAILURE);
}
