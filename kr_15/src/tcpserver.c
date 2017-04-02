#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "common.h"

#define MAX_CLIENTS 10

int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sockfd == -1) {
		perror("[TCP SERVER]: socket");
		goto error;
	}

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
		perror("[TCP SERVER]: bind");
                goto error;
	}

	if (listen(sockfd, MAX_CLIENTS) == -1) {
	        perror("[TCP SERVER]: listen");
                goto error;
        }

	// Recieving the connection from the client
	struct sockaddr_in out_addr;
	socklen_t out_addr_len;
	int out_sockfd = accept(sockfd, (struct sockaddr *)&out_addr, &out_addr_len);

	if (out_sockfd == -1) {
                perror("[TCP SERVER]: accept");
                goto error;
        }

	// Recieving the message form the client
	char buf[BUFLEN];

	memset(buf, 0, sizeof(char) * BUFLEN);
	int bytes_readed = read(out_sockfd, buf, sizeof(char) * BUFLEN);
	if (bytes_readed < 0) {
		perror("[TCP SERVER]: read");
                goto error;
	}
	printf("[TCP SERVER]: Recieved message \"%s\"\n", buf);

	// Answer, if the message is "HELLO"
	if (strcmp(buf, "HELLO") == 0) {
		sprintf(buf, "HI");
		int bytes_written = write(out_sockfd, buf, strlen(buf) + 1);
		if (bytes_written < 0) {
			perror("[TCP SERVER]: write");
	                goto error;
		}

		printf("[TCP SERVER]: Sended message \"%s\"\n", buf);
	}

	close(sockfd);
	close(out_sockfd);
	exit(EXIT_SUCCESS);

error:
	close(sockfd);
	close(out_sockfd);
        exit(EXIT_FAILURE);
}
