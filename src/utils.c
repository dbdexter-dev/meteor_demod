#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "utils.h"

/* Print usage info */
void
usage(char *pname)
{
	fprintf(stderr, "Usage: %s [options] file\n", pname);
	fprintf(stderr, "\t-r, --rate <rate>      Set the symbol rate to <rate> (default: 72000)\n"
	                "\t-o, --output <file>    Output the decoded symbols to <file>\n");
	exit(0);
}
/* Clamp a real value to a signed char */
char
clamp(float x)
{
	if (x < -128.0) {
		return -128;
	} else if (x > 127.0) {
		return 127;
	}
	return x;
}

/* Hard slicer, used to determine the closest symbol to a sample in the
 * constellation diagram  */
int
slice(float x)
{
	if (x < 0) {
		return -1;
	} else if (x > 0) {
		return 1;
	}
	return 0;
}

float complex
rotate(float complex i, float arg)
{
	return i * exp(I * arg);
}

/* Abort */
void
fatal(char *msg)
{
	fprintf(stderr, "[FATAL]: %s\n", msg);
	exit(1);
}


/* Malloc with abort on error */
void*
safealloc(size_t size)
{
	void *ptr;
	ptr = malloc(size);
	if (!ptr) {
		fatal("Failed to allocate block");
	}

	return ptr;
}
