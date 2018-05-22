#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "demod.h"
#include "options.h"
#include "utils.h"
#include "wavfile.h"

/* Default values */
#define SYM_RATE 72000
#define INTERP_FACTOR 4

#define RRC_ALPHA 0.6
#define FIR_ORDER 32

#define COSTAS_DAMP 1/M_SQRT2
#define COSTAS_BW 80
#define COSTAS_INIT_FREQ -0.005

#define AGC_WINSIZE 1024 * 8
#define AGC_TARGET 180

#define CHUNKSIZE 32768

int
main(int argc, char *argv[])
{
	int c;
	Sample *raw_samp;
	Demod *demod;

	/* Command line changeable parameters {{{*/
	int net_enabled, net_port;
	int symbol_rate;
	float costas_bw;
	unsigned interp_factor;
	char *out_fname;
	/*}}}*/
	/* Argument handling {{{ */

	/* Initialize the parameters that can be overridden with command-line args */
	optind = 0;
	net_port = -1;
	symbol_rate = SYM_RATE;
	costas_bw = COSTAS_BW;
	out_fname = NULL;
	interp_factor = INTERP_FACTOR;

	if (argc < 2) {
		splash();
		usage(argv[0]);
	}

	/* Parse command line args */
	while ((c = getopt_long(argc, argv, SHORTOPTS, longopts, NULL)) != -1) {
		switch (c) {
		case 'b':
			costas_bw = atoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			break;
		case 'o':
			out_fname = optarg;
			break;
		case 'n':
			if (net_port < 0) {
				net_port = 0;
			}
			break;
		case 'p':
			net_enabled = 1;
			net_port = atoi(optarg);
			break;
		case 'r':
			symbol_rate = atoi(optarg);
			break;
		case 's':
			interp_factor = atoi(optarg);
			break;
		case 'v':
			version();
			break;
		default:
			usage(argv[0]);
		}
	}

	if (argc - optind < 1) {
		usage(argv[0]);
	}
	/*}}}*/

	/* If no filename was specified and networking is off, generate a filename */
	if (!out_fname && !net_enabled) {
		fprintf(stderr, "Please specify a filename to output to, or -n for networking\n\n");
		usage(argv[0]);
	}

	/* Open raw samples file */
	raw_samp = open_samples_file(argv[optind]);
	if (!raw_samp) {
		fatal("Couldn't open samples file");
	}

	/* Splash screen-ish */
	splash();

	demod = demod_init(raw_samp, interp_factor, costas_bw, symbol_rate);
	demod_start(demod, net_port, out_fname);

	while (demod_status(demod)) {
		sleep(1);
		printf("(%.1f) PLL freq: %+.4f\n",
		       demod_get_perc(demod), demod_get_freq(demod));
	}

	demod_join(demod);

	/* Print some stats about our progress */
/*	bytes_out_count += 2;*/
/*	humanize(bytes_out_count, humancount);*/
/*	printf("(%.1f%%)      %s bytes written     PLL freq: %+.1f Hz \r",*/
/*			wav_get_perc(raw_samp), humancount, costas->nco_freq*symbol_rate/(2*M_PI));*/
	fflush(stdout);

	fprintf(stderr, "\n");

	return 0;
}

