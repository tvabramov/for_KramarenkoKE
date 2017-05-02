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

	// 1 sending "HELLO"
	memset(buf, 0, sizeof(char) * BUFLEN);
	sprintf(buf, "HELLO");
	int bytes_written = write(sockfd, buf, strlen(buf) + 1);
	if (bytes_written < 0) {
        	perror("[TCP CLIENT]: write");
                goto error;
	}
	printf("[TCP CLIENT]: Sended message \"%s\"\n", buf);

	// 2. Getting the new port
	unsigned short new_port = 0;
        int bytes_readed = read(sockfd, &new_port, sizeof(unsigned short));
        if (bytes_readed < 0) {
                perror("[TCP CLIENT]: read");
                goto error;
        }
	printf("[TCP CLIENT]: Recieved the new port = %d\n", (int)ntohs(new_port));

	// 3. Sending request "OK"
	memset(buf, 0, sizeof(char) * BUFLEN);
        sprintf(buf, "OK");
        bytes_written = write(sockfd, buf, strlen(buf) + 1);
        if (bytes_written < 0) {
                perror("[TCP CLIENT]: write");
                goto error;
        }
        printf("[TCP CLIENT]: Sended message \"%s\"\n", buf);

	// 4. Closing the old connection
	close(sockfd);
	memset(&srv_addr, 0, sizeof(srv_addr));

	// 5. New connecion via the new port
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sockfd == -1) {
                perror("[TCP CLIENT]: socket");
                goto error;
        }

        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port = new_port;
        if (inet_aton(SRV_IP, &srv_addr.sin_addr) == 0) {
                perror("[TCP CLIENT]: inet_aton");
                goto error;
        }

        if (connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
                perror("[TCP CLIENT]: connect");
                goto error;
        }

	printf("[TCP CLIENT]: New connection is done on the port = %d\n", (int)ntohs(srv_addr.sin_port));


	close(sockfd);
	exit(EXIT_SUCCESS);

error:
	close(sockfd);

        exit(EXIT_FAILURE);
}
