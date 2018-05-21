#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "agc.h"
#include "interpolator.h"
#include "options.h"
#include "pll.h"
#include "utils.h"
#include "wavfile.h"

/* Default values */
#define SYM_RATE 72000
#define INTERP_FACTOR 4

#define RRC_ALPHA 0.6
#define FIR_ORDER 32

#define COSTAS_DAMP 1/M_SQRT2
#define COSTAS_BW 90
#define COSTAS_INIT_FREQ -0.005

#define AGC_WINSIZE 1024 * 8
#define AGC_TARGET 200

#define CHUNKSIZE 32768

int
main(int argc, char *argv[])
{
	int i, c, count;
	Sample *raw_samp, *interp;
	Costas *costas;
	Agc *agc;
	unsigned out_fname_should_free;

	/* Output-related variables */
	size_t bytes_out_count;
	char humancount[8];
	char point[2];

	/* Early-late symbol timing recovery variables */
	float complex early, cur, late;
	float resync_period, resync_offset, resync_error;

	/* Command line changeable parameters {{{*/
	int symbol_rate;
	float costas_bw;
	unsigned interp_factor;
	char *out_fname;
	FILE *out_fd;
	/*}}}*/
	/* Argument handling {{{ */

	/* Initialize the parameters that can be overridden with command-line args */
	optind = 0;
	symbol_rate = SYM_RATE;
	costas_bw = COSTAS_BW;
	out_fname = NULL;
	out_fname_should_free = 0;
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

	if (argc- optind < 1) {
		usage(argv[0]);
	}
	/*}}}*/

	/* Splash screen-ish */
	splash();

	/* If no filename was specified, create one */
	if (!out_fname) {
		out_fname = gen_fname();
		out_fname_should_free = 1;
	}

	/* Open raw samples file */
	raw_samp = open_samples_file(argv[optind]);
	if (!raw_samp) {
		fatal("Couldn't open samples file");
	}
	/* Open output file */
	if (!(out_fd = fopen(out_fname, "w"))) {
		fprintf(stderr, "[ERROR] Couldn't open %s for output\n", out_fname);
		usage(argv[0]);
	}

	/* Initialize the AGC */
	agc = agc_init(AGC_TARGET, AGC_WINSIZE);

	/* Initialize the interpolator, associating raw_samp to it */
	interp = interp_init(raw_samp, RRC_ALPHA, FIR_ORDER, interp_factor);

	/* Initialize costas loop */
	costas_bw = 2*M_PI*costas_bw/symbol_rate;
	printf("costas bw: %f\n", costas_bw);
	costas = costas_init(COSTAS_INIT_FREQ, COSTAS_DAMP, costas_bw);

	/* Initialize the early-late timing variables */
	resync_period = interp->samplerate/(float)symbol_rate;
	resync_offset = 0;
	early = 0;
	cur = 0;

	/* Discard the first null samples */
	interp->read(interp, FIR_ORDER*interp_factor);

	/* Main processing loop */
	bytes_out_count = 0;
	while ((count = interp->read(interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			/* TODO coarse tuning with fourth-power FFT analysis */
			/* Symbol resampling (seems to be working fine)*/
			late = cur;
			cur = early;
			early = agc_apply(agc, interp->data[i]);

			resync_offset++;
			if (resync_offset >= resync_period) {
				/* The current sample is in the correct time slot, process it */
				resync_offset -= resync_period;
				resync_error = (cimag(late) - cimag(early)) * cimag(cur);
				resync_offset += (resync_error/10000*resync_period/100);

				/* Fine frequency/phase tuning (beta) */
				cur = costas_resync(costas, cur);

				/* Write binary stream to file */
				point[0] = clamp(creal(cur)/2);
				point[1] = clamp(cimag(cur)/2);
				fwrite(point, sizeof(*point), 2, out_fd);
				bytes_out_count += 2;
			}
		}
		humanize(bytes_out_count, humancount);
		fprintf(stderr, "(%.1f%%)      %s bytes written     PLL freq: %+.1f Hz          \r",
		        wav_get_perc(raw_samp), humancount, costas->nco_freq*symbol_rate/(2*M_PI));
	}
	fprintf(stderr, "\n");

	/* Cleanup */
	if (out_fd != stdout) {
		fclose(out_fd);
	}
	if (out_fname_should_free) {
		free(out_fname);
	}

	agc_free(agc);
	costas_free(costas);
	interp->close(interp);
	raw_samp->close(raw_samp);
	return 0;
}

