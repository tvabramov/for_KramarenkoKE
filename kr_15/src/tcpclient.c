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
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sockfd == -1) {
		perror("[TCP CLIENT]: socket");
		goto error;
	}

	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &srv_addr.sin_addr) == 0) {
		perror("[TCP CLIENT]: inet_aton");
		goto error;
	}

	if (connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
		perror("[TCP CLIENT]: connect");
                goto error;
	}

	char buf[BUFLEN];

	memset(buf, 0, sizeof(char) * BUFLEN);
	sprintf(buf, "HELLO");
	int bytes_written = write(sockfd, buf, strlen(buf) + 1);
	if (bytes_written < 0) {
        	perror("[TCP CLIENT]: write");
                goto error;
	}
	printf("[TCP CLIENT]: Sended message \"%s\"\n", buf);

	memset(buf, 0, sizeof(char) * BUFLEN);
        int bytes_readed = read(sockfd, buf, sizeof(char) * BUFLEN);
        if (bytes_readed < 0) {
                perror("[TCP CLIENT]: read");
                goto error;
        }
	printf("[TCP CLIENT]: Recieved message \"%s\"\n", buf);


	close(sockfd);
	exit(EXIT_SUCCESS);

error:
	close(sockfd);

        exit(EXIT_FAILURE);
}
