#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "agc.h"
#include "pll.h"
#include "interpolator.h"
#include "utils.h"
#include "wavfile.h"

/* Default values */
#define SYM_RATE 72000
#define OUTFNAME "lrpt.s"
#define INTERP_FACTOR 4

#define RRC_ALPHA 0.6
#define FIR_ORDER 32

#define COSTAS_DAMP 1/M_SQRT2
#define COSTAS_BW 0.015
#define COSTAS_INIT_FREQ -0.005

#define AGC_WINSIZE 1024 * 8
#define AGC_TARGET 200

#define CHUNKSIZE 32768

#define SHORTOPTS "hvr:s:o:"

int
main(int argc, char *argv[])
{
	int i, c, count;
	Agc *agc;
	float complex cur_sample;
	Sample *raw_samp, *interp;
	size_t bytecount;
	char humancount[8];
	char point[2];

	/* Costas loop filters and parameters */
	Costas *costas;

	/* Early-late symbol timing recovery variables */
	float complex early, late;
	float resync_period, resync_offset, resync_error;

	/* Command line changeable parameters {{{*/
	int symbol_rate;
	unsigned interp_factor;
	char *out_fname;
	FILE *out_fd;
	/*}}}*/
	/* TODO proper argument handling and stuff {{{ */
	if (argc < 2) {
		usage(argv[0]);
	}

	optind = 0;
	symbol_rate = SYM_RATE;
	out_fname = OUTFNAME;
	interp_factor = INTERP_FACTOR;
	while ((c = getopt(argc, argv, SHORTOPTS)) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			break;
		case 'r':
			symbol_rate = atoi(optarg);
			break;
		case 's':
			interp_factor = atoi(optarg);
			break;
		case 'o':
			out_fname = optarg;
			break;
		case 'v':
			version();
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind < 1) {
		usage(argv[0]);
	}
	/*}}}*/

	/* Open raw samples file */
	raw_samp = open_samples_file(argv[optind]);
	if (!raw_samp) {
		fatal("Couldn't open samples file");
	}
	/* Open output file */
	if (!(out_fd = fopen(out_fname, "w"))) {
		fprintf(stderr, "[ERROR] Couldn't open file %s for output\n", out_fname);
		usage(argv[0]);
	}

	agc = agc_init(AGC_TARGET, AGC_WINSIZE);

	/* Initialize the interpolator, associating raw_samp to it */
	interp = interp_init(raw_samp, RRC_ALPHA, FIR_ORDER, interp_factor);

	/* Initialize costas loop */
	costas = costas_init(COSTAS_INIT_FREQ, COSTAS_DAMP, COSTAS_BW);

	/* Initialize the early-late timing variables */
	resync_period = interp->samplerate/(float)symbol_rate;
	resync_offset = 0;
	early = 0;
	cur_sample = 0;

	/* Discard the first null samples */
	interp->read(interp, FIR_ORDER*interp_factor);
	
	/* Main processing loop */
	bytecount = 0;
	while ((count = interp->read(interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			/* TODO coarse tuning with fourth-power FFT analysis */
			/* Symbol resampling (seems to be working fine)*/
			late = cur_sample;
			cur_sample = early;
			early = agc_apply(agc, interp->data[i]);
/*			early = costas_resync(costas, early);*/

			resync_offset++;
			if (resync_offset >= resync_period) {
				/* The current sample is in the correct time slot, process it */
				resync_offset -= resync_period;
				resync_error = (cimag(late) - cimag(early)) * cimag(cur_sample);
				resync_offset += (resync_error/100000*resync_period/100);

				/* Fine frequency/phase tuning (beta) */
				cur_sample = costas_resync(costas, cur_sample);

/*				printf("%f %f\n", creal(cur_sample), cimag(cur_sample));*/

				/* Write binary stream to file */
				point[0] = clamp(creal(cur_sample)/2);
				point[1] = clamp(cimag(cur_sample)/2);
				fwrite(point, sizeof(*point), 2, out_fd);
				bytecount += 2;
			}
		}
		humanize(bytecount, humancount);
		fprintf(stderr, "Writing to %s: %s          \r", out_fname, humancount);
	}
	fprintf(stderr, "\n");

	/* Cleanup */
	if (out_fd != stdout) {
		fclose(out_fd);
	}
	interp->close(interp);
	raw_samp->close(raw_samp);
	return 0;
}

