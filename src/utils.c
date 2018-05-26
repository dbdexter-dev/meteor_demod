#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "utils.h"

/* Clamp a real value to a signed char */
char
clamp(float x)
{
	if (x < -128.0) {
		return -128;
	}
	if (x > 127.0) {
		return 127;
	}
	if (x > 0 && x < 1) {
		return 1;
	}
	if (x > -1 && x < 0) {
		return -1;
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

/* From byte size to human readable format */
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

/* Print usage info */
void
usage(const char *pname)
{
	splash();
	fprintf(stderr, "Usage: %s [options] file_in\n", pname);
	fprintf(stderr,
			"   -o, --output <file>     Output decoded symbols to <file>\n"
			"   -b, --pll-bw <bw>       Set the PLL bandwidth to <bw> (default: 90)\n"
	        "   -r, --rate <rate>       Set the symbol rate to <rate> (default: 72000)\n"
	        "   -O, --oversamp <mult>   Set the interpolation factor to <mult> (default: 4)\n"
	        "   -R, --refresh-rate <ms> Refresh the status screen every <ms> ms (default: 50ms in TUI mode, 5000ms in batch mode)\n"
	        "   -B, --batch             Do not use ncurses, just write the message log to stdout\n"
	        "   -q, --quiet             Do not print status information to the info log\n"
	        "\n"
	        "   -h, --help              Print this help screen\n"
	        "   -v, --version           Print version info\n"
	        );
	exit(0);
}

/* Convert seconds to a HH:MM:SS string */
void
seconds_to_str(unsigned secs, char *buf)
{
	assert(secs < (99*60*60));

	unsigned h, m, s;
	s = secs % 60;
	m = (secs / 60) % 60;
	h = secs / 3600;
	sprintf(buf, "%02u:%02u:%02u", h, m, s);
}

/* Generate a semi-unique filename */
char*
gen_fname()
{
	char *ret;
	time_t t;
	struct tm* tm;

	t = time(NULL);
	tm = localtime(&t);

	ret = safealloc(sizeof("LRPT_YYYY_MM_DD_HH_MM.s"));
	strftime(ret, sizeof("LRPT_YYYY_MM_DD_HH_MM.s"), "LRPT_%Y_%m_%d-%H_%M.s", tm);

	return ret;
}

/* Startup banner */
void
splash()
{
    printf("\nMeteor-M2 LRPT demodulator v%s\n\n", VERSION);
}


/* Print version info */
void
version()
{
	splash();
    fprintf(stderr, "Released under the GNU GPLv3\n\n");
    exit(0);
}


/* Abort */
void
fatal(const char *msg)
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
