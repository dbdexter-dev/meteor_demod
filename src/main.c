#include <complex.h>
#include <math.h>
#include <stdio.h>
#include "agc.h"
#include "pll.h"
#include "interpolator.h"
#include "nco.h"
#include "utils.h"
#include "wavfile.h"

#define SYM_RATE 72000
#define CHUNKSIZE 32768

#define RRC_ALPHA 0.6
#define FIR_ORDER 32
#define INTERP_FACTOR 8

#define COSTAS_ORD 16
#define COSTAS_WN 10
#define COSTAS_LOOP_WN 0.001
#define COSTAS_ZETA 1/M_SQRT2
#define COSTAS_GAIN 1000
#define COSTAS_ALPHA 0.01

#define AGC_WINSIZE 1024 * 16
#define AGC_TARGET 200

int
main(int argc, char *argv[])
{
	int i, count;
	float complex cur_sample, agc_out, costas_out;
	Sample *samples;
	Sample *interp;
	Filter *costas_lpf;
	Filter *costas_butt2;
	Costas *costas;
	float costas_wn;

	/* TODO proper argument handling and stuff */
	if (argc < 2) {
		usage(argv[0]);
	}

	/* Open raw samples file */
	samples = open_samples_file(argv[1]);
	if (!samples) {
		fatal("Couldn't open samples file");
	}

	agc_init(AGC_TARGET, AGC_WINSIZE);

	costas_lpf = filter_lowpass(COSTAS_ORD, COSTAS_WN);
	costas_butt2 = filter_butt2(COSTAS_ZETA, COSTAS_LOOP_WN, COSTAS_GAIN);
	costas = costas_init(costas_lpf, costas_butt2, COSTAS_ALPHA);

	interp = interp_init(samples, RRC_ALPHA, FIR_ORDER, INTERP_FACTOR);
#ifdef __DEBUG
	fprintf(stderr, "[main.c] Will interpolate %d sps to %d sps\n",
			samples->samplerate, interp->samplerate);
#endif
	/* Read the first null samples */
	count = interp->read(interp, FIR_ORDER*INTERP_FACTOR);

	while ((count = interp->read(interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			cur_sample = interp->data[i];
			/* Root-raised cosine filter (inside the interpolator) */
			/* TODO coarse tuning with fourth-power FFT analysis */
			/* AGC (wip) */
			agc_out = agc_apply(cur_sample);
/*			printf("%f %f\n", creal(agc_out), cimag(agc_out));*/
			/* Fine frequency/phase tuning (wip)*/
			costas_out = costas_resync(costas, agc_out);
			/* Interpolation (now in the while loop itself)*/
			/* TODO Symbol resampling */
			/* TODO Writing binary stream to file */
			printf("%f %f\n", creal(costas_out), cimag(costas_out));
		}
	}

	filter_free(costas_lpf);
	filter_free(costas_butt2);
	interp->close(interp);
	samples->close(samples);
	return 0;
}

