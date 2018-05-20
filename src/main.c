#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "agc.h"
#include "pll.h"
#include "interpolator.h"
#include "nco.h"
#include "utils.h"
#include "wavfile.h"

#define SYM_RATE 80000
#define CHUNKSIZE 32768

#define RRC_ALPHA 0.6
#define FIR_ORDER 32
#define INTERP_FACTOR 4

#define COSTAS_LPF_ORD 64
#define COSTAS_LPF_WN 2*M_PI*SYM_RATE
#define COSTAS_LOOP_WN 2*M_PI*0.1
#define COSTAS_LOOP_ZETA 1/M_SQRT2
#define COSTAS_LOOP_GAIN 1000
#define COSTAS_ALPHA 0.001

#define AGC_WINSIZE 1024 * 8
#define AGC_TARGET 200
#define SHORTOPTS "hvr:s:"

int
main(int argc, char *argv[])
{
	int i, c, count;
	Agc *agc;
	float complex cur_sample;
	Sample *raw_samp, *interp;

	/* Costas loop filters and parameters */
	Filter *costas_lpf;
	Filter *costas_butt2;
	Costas *costas;
	float costas_loop_wn, costas_lpf_wn;

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
	out_fname = NULL;
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
	/* Open possible output file */
	if (out_fname) {
		if (!(out_fd = fopen(out_fname, "w"))) {
			fprintf(stderr, "[ERROR] Couldn't open file %s for output\n", out_fname);
			usage(argv[0]);
		}
	} else {
		out_fd = stdout;
	}

	agc = agc_init(AGC_TARGET, AGC_WINSIZE);

	/* Initialize the interpolator, associating raw_samp to it */
	interp = interp_init(raw_samp, RRC_ALPHA, FIR_ORDER, interp_factor);

	/* Calculate the digital cutoff frequencies */
	costas_lpf_wn = COSTAS_LPF_WN/(SYM_RATE);
	costas_loop_wn = COSTAS_LOOP_WN/(SYM_RATE);

	/* Initialize a Costas object with simple low-pass arm filters and a 2nd order
	 * Butterworth loop filter */
	costas_lpf = filter_lowpass(COSTAS_LPF_ORD, costas_lpf_wn);
/*	costas_butt2 = filter_lowpass(COSTAS_LPF_ORD, costas_loop_wn);*/
	costas_butt2 = filter_butt2(COSTAS_LOOP_ZETA, costas_loop_wn, COSTAS_LOOP_GAIN);
	costas = costas_init(costas_lpf, costas_butt2, COSTAS_ALPHA);

	/* Initialize the early-late timing variables */
	resync_period = interp->samplerate/(float)symbol_rate;
	resync_offset = 0;
	early = 0;
	cur_sample = 0;

	/* Discard the first null samples */
	interp->read(interp, FIR_ORDER*interp_factor);
	
	/* Main processing loop */
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
				cur_sample = costas_resync(costas, cur_sample);

				/* Fine frequency/phase tuning (wip)*/
/*				printf("%f %f\n", creal(cur_sample), cimag(cur_sample));*/

				/* TODO Write binary stream to file as soon as the PLL locks */
			}
		}
	}

	/* Cleanup */
	if (out_fd != stdout) {
		fclose(out_fd);
	}
	filter_free(costas_lpf);
	filter_free(costas_butt2);
	interp->close(interp);
	raw_samp->close(raw_samp);
	return 0;
}

