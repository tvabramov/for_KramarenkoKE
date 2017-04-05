#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "clinit.h"

void printHelp()
{
	printf("Text chat, that uses messages queue of IPC. Author: Timofey V. Abramov.\n");
	printf("Arguments:\n");
	printf("  -h --help\t\t\t\tPrint help information\n");
	printf("  -s --is-server\t\t\tRun chat in server mode\n");
	printf("  -n --nickname <string>\t\tSpecify user's nickname\n");
	printf("Notes:\n");
	printf("  <...> - mandatory parameter\n");
}

/**
\brief      Function that initialize the startup parameters.
\details    This functions parses "argv" parameters.
\param[in]  argc Amount of elements in argv array.
\param[in]  argv Launch parameters.
\param[out] clinit_params Pointer to CLINIT_PARAMS struct.
\return     Error flag. If 0, minimal required parameters are passed,
            otherwise - not 0 is returned.
*/
int clinit(int argc, char **argv, CLINIT_PARAMS *clinit_params)
{
        (*clinit_params).i_am_server = 0;
	sprintf((*clinit_params).nickname, "User");

        while (1)
        {
                static struct option long_options[] =
                {
                        {"help", no_argument, 0, 'h'},
			{"is-server", no_argument, 0, 's'},
                        {"nickname", required_argument, 0, 'n'},

                        {0, 0, 0, 0}
                };

                int indexptr = 0;
                int c = getopt_long(argc, argv, "hsn:", long_options, &indexptr);

                if (c == -1) break;

                switch (c)
                {
                        case 'h':
				printHelp();
                                exit(EXIT_SUCCESS);
			case 's':
				(*clinit_params).i_am_server = 1;
				break;
                        case 'n':
				if (strlen(optarg) <= MAX_NICKNAME_LEN) {
                                	sprintf((*clinit_params).nickname, optarg);
				} else {
					memcpy((*clinit_params).nickname, optarg, sizeof(char) * MAX_NICKNAME_LEN);
					(*clinit_params).nickname[MAX_NICKNAME_LEN] = '\0';
				}
                                break;
                        case '?':
                                fprintf(stderr, "Error: initialization error\n");
                                exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
        }

	return 0;
}

void print_clinit_params(CLINIT_PARAMS clinit_params)
{
	printf("Launch parameters:\n");
	(clinit_params.i_am_server == 1) ? printf("\tServer mode\n") : printf("\tClient mode\n");
	printf("\tNickname = %s\n", clinit_params.nickname);
}
