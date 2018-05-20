#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "utils.h"

#define VERSION "0.1a"

/* Print usage info */
void
usage(char *pname)
{
    fprintf(stderr, "\n\t~ Meteor-M2 LRPT demodulator v%s ~\n\n", VERSION);
	fprintf(stderr, "Usage: %s [options] file_in\n", pname);
	fprintf(stderr, 
	        "   -h, --help             Print this help screen\n"
	        "   -r, --rate <rate>      Set the symbol rate to <rate> (default: 72000)\n"
	        "   -s, --oversamp <mult>  Set the interpolator oversampling multiplier to <mult> (default: 4)\n"
	        "   -v, --version          Print version info\n"
	        );
	exit(0);
}

void
version()
{
    fprintf(stderr, "Meteor_demod v%s\nReleased under the GNU GPLv3\n\n", VERSION);
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
