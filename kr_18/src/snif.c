#include <stdio.h> 
#include <stdlib.h>
#include <errno.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <pcap.h>
#include <string.h>
#include "clinit.h"
#include "net_headers.h"

#define SNAPLENMAX 65535
#define PRINT_BYTES_PER_LINE 16

int print_devices_list();
void print_data_hex(const uint8_t* data, int size);
void handle_packet(uint8_t* user, const struct pcap_pkthdr *hdr, const uint8_t* bytes);

int main(int argc, char **argv)
{
	char *device, *filter;
	int print_dev_list;

	if (clinit(argc, argv, &device, &filter, &print_dev_list) != 0) {
		fprintf(stderr, "Initialization error. Use '-h' key to get help");
		goto error;
	}

	//print_clinit_params(device, filter, print_dev_list);

	// If we just want to get list of devices
	if (print_dev_list == 1) {
		if (print_devices_list() == 0) {
			free(device);
			free(filter);
			exit(EXIT_SUCCESS);
		} else {
			goto error;
		}
	}

	// Open device in promiscuous mode 
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* descr = pcap_open_live(device, SNAPLENMAX, 1, -1, errbuf); 
	if (descr == NULL) {
		fprintf(stderr, "pcap_open_live(): %s\n", errbuf);
		goto error;
	}

	// Compiling the filter
	struct bpf_program filterprog;
	if (pcap_compile(descr, &filterprog, filter, 0, PCAP_NETMASK_UNKNOWN) == -1) {
		fprintf(stderr, "pcap_compile(): %s\n", pcap_geterr(descr));
		goto error;
	}

	if (pcap_setfilter(descr, &filterprog) == -1) {
		fprintf(stderr, "pcap_setfilter(): %s\n", pcap_geterr(descr));
		goto error;
	}

	printf("Listening %s, filter: %s...\n", device, filter);

	int res = pcap_loop(descr, -1, handle_packet, NULL);
	printf("pcap_loop returned %d\n", res);

	pcap_close(descr);
	free(device);
	free(filter);
        exit(EXIT_SUCCESS);

error:
	pcap_close(descr);
	exit(EXIT_FAILURE);
}

int print_devices_list()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_if_t *alldevsp;
	if (pcap_findalldevs(&alldevsp, errbuf)) {
		fprintf(stderr, "Error: %s", errbuf);
		goto error;
	}

	pcap_if_t *d;
	printf("\nDevices:\n");
	int i = 0;
	for (d = alldevsp; d != NULL; d = d->next) {
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}

	pcap_freealldevs(alldevsp);
	return 0;

error:
	return -1;
}

void print_data_hex(const uint8_t* data, int size)
{
	int offset = 0;
	int nlines = size / PRINT_BYTES_PER_LINE;
	if (nlines * PRINT_BYTES_PER_LINE < size)
		nlines++;

	printf("        ");

	int i;
	for (i = 0; i < PRINT_BYTES_PER_LINE; i++)
		printf("%02X ", i);

	printf("\n\n");

	int line;
	for (line = 0; line < nlines; line++) {

		printf("%04X    ", offset);
		int j;
		for (j = 0; j < PRINT_BYTES_PER_LINE; j++) {
			if (offset + j >= size)
				printf("   ");
			else
				printf("%02X ", data[offset + j]);
		}

		printf("   ");

		for (j = 0; j < PRINT_BYTES_PER_LINE; j++) {
			if(offset + j >= size)
				printf(" ");
			else if(data[offset + j] > 31 && data[offset + j] < 127)
				printf("%c", data[offset + j]);
			else
				printf(".");
		}

		offset += PRINT_BYTES_PER_LINE;
		printf("\n");
	}
}

void handle_packet(uint8_t* user, const struct pcap_pkthdr *hdr, const uint8_t* bytes)
{
	struct iphdr* ip_header = (struct iphdr*)(bytes + sizeof(struct ethhdr));
	struct sockaddr_in source, dest;

 	memset(&source, 0, sizeof(source));
	memset(&dest, 0, sizeof(dest));
	source.sin_addr.s_addr = ip_header->saddr;
	dest.sin_addr.s_addr = ip_header->daddr;

	char source_ip[128];
	char dest_ip[128];
	strncpy(source_ip, inet_ntoa(source.sin_addr), sizeof(source_ip));
	strncpy(dest_ip, inet_ntoa(dest.sin_addr), sizeof(dest_ip));

	int source_port = 0;
	int dest_port = 0;
	int data_size = 0;
	int ip_header_size = ip_header->ihl * 4;
	char* next_header = (char*)ip_header + ip_header_size;

	if (ip_header->protocol == IP_HEADER_PROTOCOL_TCP) {
		struct tcphdr* tcp_header = (struct tcphdr*)next_header;
		source_port = ntohs(tcp_header->source);
		dest_port = ntohs(tcp_header->dest);
		int tcp_header_size = tcp_header->doff * 4;
		data_size = hdr->len - sizeof(struct ethhdr) - ip_header_size - tcp_header_size;
	} else if(ip_header->protocol == IP_HEADER_PROTOCOL_UDP) {
		struct udphdr* udp_header = (struct udphdr*)next_header;
		source_port = ntohs(udp_header->source);
		dest_port = ntohs(udp_header->dest);
		data_size = hdr->len - sizeof(struct ethhdr) - ip_header_size - sizeof(struct udphdr);
	}

	printf("\n%s:%d -> %s:%d, %d (0x%x) bytes\n\n", source_ip, source_port, dest_ip, dest_port, data_size, data_size);

	if (data_size > 0) {
		int headers_size = hdr->len - data_size;
		print_data_hex(bytes + headers_size, data_size);
	}
}

