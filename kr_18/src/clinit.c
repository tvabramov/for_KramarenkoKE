#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "clinit.h"

void printHelp()
{
	printf("Network sniffer. Author: Timofey V. Abramov.\n");
	printf("Arguments:\n");
	printf("  -h --help\t\t\tPrint help information\n");
	printf("  -d --device <string>\t\tSpecify device to sniff\n");
	printf("  -f --filter <string>\t\tSpecify filter string\n");
	printf("  -l --list\t\t\tPrint list of the devices\n");
	printf("Notes:\n");
	printf("  <...> - mandatory parameter\n");
}

/**
\brief      Function that initialize the startup parameters.
\details    This functions parses "argv" parameters.
\param[in]  argc Amount of elements in argv array.
\param[in]  argv Launch parameters.
\param[out] device what device we want to sniff
\param[out] filter string-filter
\return     Error flag. If 0, minimal required parameters are passed,
            otherwise - not 0 is returned.
*/
int clinit(int argc, char **argv, char** device, char** filter, int* print_dev_list)
{
        (*device) = (char *)malloc(sizeof(char));
	(*device)[0] = '\0';

	(*filter) = (char *)malloc(sizeof(char));
	(*filter)[0] = '\0';

	*print_dev_list = 0;

        while (1)
        {
                static struct option long_options[] =
                {
                        {"help", no_argument, 0, 'h'},
			{"device", required_argument, 0, 'd'},
                        {"filter", required_argument, 0, 'f'},
			{"list", no_argument, 0, 'l'},

                        {0, 0, 0, 0}
                };

                int indexptr = 0;
                int c = getopt_long(argc, argv, "hd:f:l", long_options, &indexptr);

                if (c == -1) break;

                switch (c)
                {
                        case 'h':
				printHelp();
                                exit(EXIT_SUCCESS);
			case 'd':
				free(*device);
				*device = (char *)malloc(sizeof(char) * (strlen(optarg) + 1));
				memcpy(*device, optarg, sizeof(char) * (strlen(optarg) + 1));
				break;
                        case 'f':
				free(*filter);
				*filter = (char *)malloc(sizeof(char) * (strlen(optarg) + 1));
				memcpy(*filter, optarg, sizeof(char) * (strlen(optarg) + 1));
                                break;
			case 'l':
				*print_dev_list = 1;
				break;
                        case '?':
                                fprintf(stderr, "Error: initialization error\n");
                                return 1;
                        default:
                                abort();
                }
        }

	if (optind < argc)
        {
                fprintf(stderr, "Error: non-option ARGV-elements:");
                while (optind < argc)
                        printf (" %s", argv[optind++]);
                fprintf(stderr, "\nUse \"--help\" or \"-h\" key to get help\n");
                return 1;
        }

	return ((*device && *filter && (strlen(*device) > 0)) || (*print_dev_list == 1)) ? 0 : 1;
}

void print_clinit_params(const char* device, const char* filter, int print_dev_list)
{
	printf("Launch parameters:\n");
	if (print_dev_list == 1) {
		printf("\tPrint list of the devices\n");
	} else {
		printf("\tDevice = %s\n", device);
		printf("\tFilter = %s\n", filter);
	}
}
