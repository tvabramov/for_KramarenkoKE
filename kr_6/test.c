#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char** argv)
{
	srand(time(0));
	int retval = rand() % 100;
	printf("I'm test program, I'm going to return value = %d\n", retval);

	return retval;
}
