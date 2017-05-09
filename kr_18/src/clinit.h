#ifndef _CLINIT_H_
#define _CLINIT_H_

int clinit(int argc, char **argv, char** device, char** filter, int* print_dev_list);
void print_clinit_params(const char* device, const char* filter, int print_dev_list);

#endif
