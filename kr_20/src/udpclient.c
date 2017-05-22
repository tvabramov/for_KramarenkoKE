#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "common.h"

int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

	if (sockfd == -1) {
		perror("[UDP CLIENT]: socket");
		goto error;
	}

	int on = 1;
	setsockopt(sockfd, IPPROTO_IP /*IPPROTO_UDP*/, IP_HDRINCL, &on, sizeof(on));

	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &srv_addr.sin_addr) == 0) {
		perror("[UDP CLIENT]: inet_aton");
		goto error;
	}

	// Sending the message to the server
	char buf[BUFLEN + sizeof(struct udphdr) + sizeof(struct iphdr)];
	memset(buf, 0, BUFLEN + sizeof(struct udphdr) + sizeof(struct iphdr));

	// setting the ip header
	struct iphdr ip_header;
	memset(&ip_header, 0, sizeof(struct iphdr));
	ip_header.ihl = (sizeof(struct iphdr) / 4);
	ip_header.version = 4;
	ip_header.tos = 0;
	ip_header.tot_len = htons(BUFLEN + sizeof(struct udphdr) + sizeof(struct iphdr));
	ip_header.id = 0;
	ip_header.frag_off = 0;
	ip_header.ttl = 8;
	ip_header.protocol = IPPROTO_UDP;
	ip_header.check = 0;
	ip_header.saddr = inet_addr("127.0.0.1");
	ip_header.daddr = inet_addr("127.0.0.1");
	memcpy(buf, &ip_header, sizeof(struct iphdr));

	// setting the udp header
	struct udphdr udp_header;
	memset(&udp_header, 0, sizeof(struct udphdr));
	udp_header.source = htons(PORT);
	udp_header.dest = htons(PORT);
	udp_header.len = htons(BUFLEN + sizeof(struct udphdr));
	udp_header.check = 0;
	memcpy(buf + sizeof(struct iphdr), &udp_header, sizeof(struct udphdr));

	// setting the message
	sprintf(buf + sizeof(struct iphdr) + sizeof(struct udphdr), "HELLO");
	if (sendto(sockfd, buf, BUFLEN + sizeof(struct iphdr) + sizeof(struct udphdr), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
		perror("[UDP CLIENT]: sendto");
		goto error;
	}
	printf("[UDP CLIENT]: Sended message \"%s\"\n", buf + sizeof(struct iphdr) + sizeof(struct udphdr));

	socklen_t srv_addr_len = sizeof(srv_addr);
	memset(buf, 0, BUFLEN + sizeof(struct udphdr) + sizeof(struct iphdr));

        if (recvfrom(sockfd, buf, BUFLEN + sizeof(struct udphdr) + sizeof(struct iphdr), 0, (struct sockaddr *)&srv_addr, &srv_addr_len) == -1) {
                perror("[UDP CLIENT]: recvfrom");
                goto error;
        }
        printf("[UDP CLIENT]: Recieved message \"%s\"\n", buf + sizeof(struct udphdr) + sizeof(struct iphdr));

	if (recvfrom(sockfd, buf, BUFLEN + sizeof(struct udphdr) + sizeof(struct iphdr), 0, (struct sockaddr *)&srv_addr, &srv_addr_len) == -1) {
                perror("[UDP CLIENT]: recvfrom");
                goto error;
        }
        printf("[UDP CLIENT]: Recieved message \"%s\"\n", buf + sizeof(struct udphdr) + sizeof(struct iphdr));

	close(sockfd);
	exit(EXIT_SUCCESS);

error:
	close(sockfd);

        exit(EXIT_FAILURE);
}
