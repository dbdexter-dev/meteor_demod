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
#define INTERP_FACTOR 8

#define COSTAS_ORD 16
#define COSTAS_WN 2*M_PI*10
#define COSTAS_LOOP_WN 2*M_PI*10
#define COSTAS_ZETA 1/M_SQRT2
#define COSTAS_GAIN 1000
#define COSTAS_ALPHA 0.01

#define AGC_WINSIZE 1024 * 16
#define AGC_TARGET 200
#define SHORTOPTS "r:o:"

int
main(int argc, char *argv[])
{
	int i, c, count;
	int symbol_rate;
	float complex cur_sample, agc_out, costas_out;
	Sample *raw_samp, *interp;
	Filter *costas_lpf, *costas_butt2;
	Costas *costas;
	float costas_loop_wn, costas_lpf_wn;

	float complex early, late;
	float resync_period, resync_offset, resync_error;

	if (argc < 2) {
		usage(argv[0]);
	}

	optind = 0;
	/* TODO proper argument handling and stuff */
	symbol_rate = SYM_RATE;
	while ((c = getopt(argc, argv, SHORTOPTS)) != -1) {
		switch (c) {
		case 'r':
			symbol_rate = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind < 1) {
		usage(argv[0]);
	}

	/* Open raw samples file */
	raw_samp = open_samples_file(argv[optind]);
	if (!raw_samp) {
		fatal("Couldn't open samples file");
	}

	agc_init(AGC_TARGET, AGC_WINSIZE);

	interp = interp_init(raw_samp, RRC_ALPHA, FIR_ORDER, INTERP_FACTOR);
#ifdef __DEBUG
	fprintf(stderr, "[main.c] Will interpolate %d sps to %d sps\n",
			raw_samp->samplerate, interp->samplerate);
#endif

	costas_loop_wn = COSTAS_LOOP_WN/(interp->samplerate);
	costas_lpf_wn = COSTAS_WN/(interp->samplerate);

	costas_lpf = filter_lowpass(COSTAS_ORD, costas_lpf_wn);
	costas_butt2 = filter_butt2(COSTAS_ZETA, costas_loop_wn	, COSTAS_GAIN);
	costas = costas_init(costas_lpf, costas_butt2, COSTAS_ALPHA);

	resync_period = interp->samplerate/(float)symbol_rate;
	resync_offset = 0;
	early = 0;
	cur_sample = 0;

	/* Discard the first null samples */
	interp->read(interp, FIR_ORDER*INTERP_FACTOR);
	while ((count = interp->read(interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			late = cur_sample;
			cur_sample = early;
			early = agc_apply(interp->data[i]);

			resync_offset++;
			if (resync_offset >= resync_period) {
				resync_offset -= resync_period;
				resync_error = (creal(late) - creal(early)) * creal(cur_sample);
				if (resync_error > 255) {
					resync_offset += (resync_period/20);
				} else if (resync_error < -255) {
					resync_offset -= (resync_period/20);
				}

				/* Root-raised cosine filter (inside the interpolator) */
				/* TODO coarse tuning with fourth-power FFT analysis */
				/* AGC (wip) */
	/*			printf("%f %f\n", creal(agc_out), cimag(agc_out));*/
				/* Fine frequency/phase tuning (wip)*/
				costas_out = costas_resync(costas, cur_sample);
				/* Interpolation (now in the while loop itself)*/
				/* TODO Symbol resampling */
				/* TODO Writing binary stream to file */
				printf("%f %f %f %f\n", 
					   creal(cur_sample), cimag(cur_sample),
					   creal(costas_out), cimag(costas_out));
			}
		}
	}

	filter_free(costas_lpf);
	filter_free(costas_butt2);
	interp->close(interp);
	raw_samp->close(raw_samp);
	return 0;
}

