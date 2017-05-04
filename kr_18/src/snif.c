#include <stdio.h> 
#include <stdlib.h>
#include <errno.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <pcap.h>
#include <string.h>

#define SNAPLENMAX 65535

int main(int argc, char **argv)
{
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_if_t *alldevsp;
	if (pcap_findalldevs(&alldevsp, errbuf)) {
		printf("Error: %s", errbuf);
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
/*
	i = 0;
	for (d = alldevsp; d != NULL; d = d->next) {
		if (strcmp(d->name, "lo") == 0)
			break;
		++i;
	}

	pcap_if_t *device = NULL;
	if (d != NULL) {
		device = d;
		printf("lo device is found (N %d), connecting...\n", i + 1);
	} else {
		printf("lo device is not found, abort\n");
		goto error;
	}

	// Get the device's name
	char *dev = pcap_lookupdev(errbuf);

	if (dev == NULL) {
		fprintf(stderr, "%s\n", errbuf);
		goto error;
	}

	// Getting address and mask
	bpf_u_int32 maskp;
	bpf_u_int32 netp; 
	pcap_lookupnet(dev, &netp, &maskp, errbuf); 


	// Open device in promiscuous mode 
	pcap_t* descr = pcap_open_live(device->name, SNAPLENMAX, 1, -1, errbuf); 
	if (descr == NULL) {
		printf("pcap_open_live(): %s\n", errbuf);
		//goto error;
	}
*/

	for (d = alldevsp; d != NULL; d = d->next)
		if (pcap_open_live(d->name, SNAPLENMAX, 1, -1, errbuf) == NULL) {
			printf("pcap_open_live(): %s\n", errbuf);
		}

	pcap_freealldevs(alldevsp);
        exit(EXIT_SUCCESS);

error:
	exit(EXIT_FAILURE);
}
 

