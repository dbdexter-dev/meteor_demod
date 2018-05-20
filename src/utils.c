#include <assert.h>
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
	fprintf(stderr, "Usage: %s [options] file\n", pname);
	fprintf(stderr, 
			"   -o, --output           Output decoded symbols to <file> (default: lrpt.s)\n"
	        "   -h, --help             Print this help screen\n"
	        "   -r, --rate <rate>      Set the symbol rate to <rate> (default: 72000)\n"
	        "   -s, --oversamp <mult>  Set the interpolator oversampling factor to <mult> (default: 4)\n"
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

/* Clamp a real value in the range [-max_abs, max_abs] */
float
float_clamp(float x, float max_abs)
{
	if (x > max_abs) {
		return max_abs;
	} else if (x < -max_abs) {
		return -max_abs;
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

void
humanize(size_t count, char *buf)
{
	const char suffix[] = "bkMGTPE";
	float fcount;
	int exp_3;

	assert(buf);
	if (count < 1000) {
		sprintf(buf, "%lu %c", count, suffix[0]);
	} else {
		for (exp_3 = 0, fcount = count; fcount > 1000; fcount /= 1000, exp_3++)
			;
		if (fcount > 99.9) {
			sprintf(buf, "%3.f %c", fcount, suffix[exp_3]);
		} else if (fcount > 9.99) {
			sprintf(buf, "%3.1f %c", fcount, suffix[exp_3]);
		} else {
			sprintf(buf, "%3.2f %c", fcount, suffix[exp_3]);
		}
	}
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
