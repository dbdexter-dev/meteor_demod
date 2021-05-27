#include <stdio.h>
#include <time.h>
#include "utils.h"

static char _generated_fname[sizeof("LRPT_YYYY_MM_DD_HH_MM.s") + 1];

char*
gen_fname()
{
	time_t t;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	strftime(_generated_fname, sizeof(_generated_fname), "LRPT_%Y_%m_%d-%H_%M.s", tm);

	return _generated_fname;
}

void
humanize(size_t count, char *buf)
{
	const char suffix[] = " kMGTPE";
	float fcount;
	int exp_3;

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

void
seconds_to_str(unsigned secs, char *buf)
{
	unsigned h, m, s;

	if (secs > 99*60*60) {
		sprintf(buf, "00:00:00");
		return;
	}

	s = secs % 60;
	m = (secs / 60) % 60;
	h = secs / 3600;
	sprintf(buf, "%02u:%02u:%02u", h, m, s);
}

float
human_to_float(const char *human)
{
	int ret;
	float tmp;
	const char *suffix;

	tmp = atof(human);

	/* Search for the suffix */
	for (suffix=human; (*suffix >= '0' && *suffix <= '9') || *suffix == '.'; suffix++);

	switch(*suffix) {
		case 'k':
		case 'K':
			ret = tmp * 1000;
			break;
		case 'M':
			ret = tmp * 1000000;
			break;
		default:
			ret = tmp;
			break;
	}

	return ret;

}

void
usage(const char *pname)
{
	fprintf(stderr, "Usage: %s [options] file_in\n", pname);
	fprintf(stderr,
	        "   -B, --batch             Disable TUI and all control characters (aka \"script-friendly mode\")\n"
	        "   -m, --mode <mode>       Specify the signal modulation scheme (default: qpsk, valid modes: qpsk, oqpsk)\n"
	        "   -o, --output <file>     Output decoded symbols to <file>\n"
	        "   -q, --quiet             Do not print status information\n"
	        "   -r, --symrate <rate>    Set the symbol rate to <rate> (default: 72000)\n"
	        "   -R, --refresh-rate <ms> Refresh the status screen every <ms> ms (default: 50ms in TUI mode, 2000ms in batch mode)\n"
	        "   -s, --samplerate <samp> Force the input samplerate to <samp> (default: auto)\n"
	        "       --bps <bps>         Force the input bits per sample to <bps> (default: 16)\n"
	        "       --stdout            Write output symbols to stdout (implies -B, -q)\n"
	        "\n"
	        "   -h, --help              Print this help screen\n"
	        "   -v, --version           Print version info\n"
	        "\n"
	        "Advanced options:\n"
	        "   -b, --pll-bw <bw>       Set the PLL bandwidth to <bw> (default: 1)\n"
	        "   -d, --freq-delta <freq> Set the maximum carrier deviation to <freq> (default: +-3.5kHz)\n"
	        "   -f, --fir-order <ord>   Set the RRC filter order to <ord> (default: 32)\n"
	        "   -O, --oversamp <mult>   Set the interpolation factor to <mult> (default: 5)\n"
	        );
}

void
version()
{
	fprintf(stderr, "meteor_demod v" VERSION "\n");
}
