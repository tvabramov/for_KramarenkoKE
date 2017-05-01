#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include "common.h"

#define MAX_CLIENTS 10

void setNewTCPConnection(int *sockfd, struct sockaddr_in *addr, int port);
void startTCPServer(int server_sockfd, int client_sockfd, struct sockaddr_in server_addr);
void setNewUDPConnection(int *sockfd, struct sockaddr_in *addr, int port);
void startUDPServer(int server_sockfd, struct sockaddr_in server_addr);
void TCPconnector();
void UDPconnector();

int main(int argc, char **argv)
{
	// Fork for tcp and udp connection
	int pid = fork();

	// Error
	if (pid == -1) {
		perror("fork (TCP and UDP connection)");
		exit(EXIT_FAILURE);
	}

	if (pid != 0) {

		// Master
		TCPconnector();
	} else {

		// Child
		UDPconnector();
	}

	exit(EXIT_SUCCESS);
}

// IN = port (0 to set default);
// OUT = sockfd, addr
void setNewTCPConnection(int *sockfd, struct sockaddr_in *addr, int port)
{
	*sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (*sockfd == -1) {
                perror("socket (TCP)");
                exit(EXIT_FAILURE);
        }

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(*sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1) {
		perror("bind (TCP)");
		close(*sockfd);
		exit(EXIT_FAILURE);
	}

	// It is to know, to which port we connected, if it was 0
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if (getsockname(*sockfd, (struct sockaddr *)addr, &addrlen) == -1) {
		perror("getsockname (TCP)");
		close(*sockfd);
		exit(EXIT_FAILURE);
	}

	if (listen(*sockfd, MAX_CLIENTS) == -1) {
		perror("listen (TCP)");
		close(*sockfd);
		exit(EXIT_FAILURE);
	}
}

void startTCPServer(int server_sockfd, int client_sockfd, struct sockaddr_in server_addr)
{
	// empty now
}

// IN = port (0 to set default);
// OUT = sockfd, addr
void setNewUDPConnection(int *sockfd, struct sockaddr_in *addr, int port)
{
	*sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (*sockfd == -1) {
		perror("socket (UDP)");
                exit(EXIT_FAILURE);
	}

        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        addr->sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(*sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1) {
                perror("bind (TCP)");
                close(*sockfd);
                exit(EXIT_FAILURE);
        }

	// It is to know, to which port we connected, if it was 0
        socklen_t addrlen = sizeof(struct sockaddr_in);
        if (getsockname(*sockfd, (struct sockaddr *)addr, &addrlen) == -1) {
                perror("getsockname (UDP)");
                close(*sockfd);
                exit(EXIT_FAILURE);
        }
}

void startUDPServer(int server_sockfd, struct sockaddr_in server_addr)
{
        // empty now
}

void TCPconnector()
{
	int tcp_sockfd;
	struct sockaddr_in my_tcp_addr;

	setNewTCPConnection(&tcp_sockfd, &my_tcp_addr, PORT);
	
	for ( ; ; ) {

		// Recieving the connection form the client
		struct sockaddr_in out_addr;
		socklen_t out_addr_len;
		int out_sockfd = accept(tcp_sockfd, (struct sockaddr *)&out_addr, &out_addr_len);

		if (out_sockfd == -1) {
			perror("accept (TCP)");
			goto tcp_error;
		}

		// Recieving the message form the client
		char buf[BUFLEN];

		memset(buf, 0, sizeof(char) * BUFLEN);
		int bytes_readed = read(out_sockfd, buf, sizeof(char) * BUFLEN);
		if (bytes_readed < 0) {
			perror("read (TCP)");
			goto tcp_error;
		}
		printf("[TCP SERVER]: Recieved message \"%s\"\n", buf);

		// Answer, if the message is "HELLO"
		if (strcmp(buf, "HELLO") == 0) {

			int pid = fork();

			// Error
			if (pid == -1) {
				perror("fork (TCP client connection)");
				exit(EXIT_FAILURE);
			}

			if (pid != 0) {

				// Master, do nothing
			} else {

				// Child

				// 1. Start the new connection, port is set automatically
				int new_tcp_sockfd;
				struct sockaddr_in new_my_tcp_addr;
				setNewTCPConnection(&new_tcp_sockfd, &new_my_tcp_addr, 0);

				// 2. Send the port over old connection
				memcpy(buf, &new_my_tcp_addr.sin_port, sizeof(unsigned short));
				int bytes_written = write(out_sockfd, buf, sizeof(unsigned short));
				if (bytes_written < 0) {
					perror("write (TCP)");
					close(new_tcp_sockfd);
					goto tcp_error;
				}
				printf("[TCP SERVER]: Sended the new port = %d\n", (int)ntohs(new_my_tcp_addr.sin_port));

				// 3. Check the request, must be "OK"
				bytes_readed = read(out_sockfd, buf, sizeof(char) * BUFLEN);
				if (bytes_readed < 0) {
					perror("read (TCP)");
					close(new_tcp_sockfd);
					goto tcp_error;
				}
				printf("[TCP SERVER]: Recieved message \"%s\"\n", buf);

				if (strcmp(buf, "OK") != 0) {
					fprintf(stderr, "Invalid request from client\n");
					close(new_tcp_sockfd);
					goto tcp_error;
				}

				// 4. If OK, close main connection and accept the new one
				close(tcp_sockfd);
				tcp_sockfd = new_tcp_sockfd;
				my_tcp_addr = new_my_tcp_addr;
				out_sockfd = accept(tcp_sockfd, (struct sockaddr *)&out_addr, &out_addr_len);

				if (out_sockfd == -1) {
					perror("accept (TCP)");
					goto tcp_error;
				}

				printf("[TCP SERVER]: New connection is done on the port = %d\n", (int)ntohs(my_tcp_addr.sin_port));

				startTCPServer(tcp_sockfd, out_sockfd, my_tcp_addr);
			}
		}
	}

	close(tcp_sockfd);
	exit(EXIT_SUCCESS);

tcp_error:
	close(tcp_sockfd);
	exit(EXIT_FAILURE);		
}


void UDPconnector()
{
	int udp_sockfd;
        struct sockaddr_in my_udp_addr;

        setNewUDPConnection(&udp_sockfd, &my_udp_addr, PORT);

	for ( ; ; ) {

		// Recieving the message from the client
		char buf[BUFLEN];

		struct sockaddr_in out_addr;
		socklen_t out_addr_len = sizeof(out_addr);
		memset(buf, 0, BUFLEN);
		if (recvfrom(udp_sockfd, buf, BUFLEN, 0, (struct sockaddr *)&out_addr, &out_addr_len) == -1) {
			perror("recvfrom (UDP)");
			goto udp_error;
		}
		printf("[UDP SERVER]: Recieved message \"%s\"\n", buf);


		// Answer, if the message is "HELLO"
                if (strcmp(buf, "HELLO") == 0) {

                        int pid = fork();

                        // Error
                        if (pid == -1) {
                                perror("fork (UDP client connection)");
                                exit(EXIT_FAILURE);
                        }

                        if (pid != 0) {

                                // Master, do nothing
                        } else {

                                // Child

				// 1. Start the new connection, port is set automatically
                                int new_udp_sockfd;
                                struct sockaddr_in new_my_udp_addr;
                                setNewUDPConnection(&new_udp_sockfd, &new_my_udp_addr, 0);

                                // 2. Send the port over old connection
                                memcpy(buf, &new_my_udp_addr.sin_port, sizeof(unsigned short));
                		if (sendto(udp_sockfd, buf, sizeof(unsigned short), 0, (struct sockaddr *)&out_addr, sizeof(out_addr)) == -1) {
		                        perror("[UDP SERVER]: sendto");
					close(new_udp_sockfd);
		                        goto udp_error;
		                }
                                printf("[UDP SERVER]: Sended the new port = %d\n", (int)ntohs(new_my_udp_addr.sin_port));

                                // 3. If OK, close main connection and accept the new one
                                close(udp_sockfd);
                                udp_sockfd = new_udp_sockfd;
                                my_udp_addr = new_my_udp_addr;

                                printf("[UDP SERVER]: New connection is done on the port = %d\n", (int)ntohs(my_udp_addr.sin_port));

                                startUDPServer(udp_sockfd, my_udp_addr);
                        }
                }
	}

	close(udp_sockfd);
	exit(EXIT_SUCCESS);

udp_error:
	close(udp_sockfd);
	exit(EXIT_FAILURE);
}

