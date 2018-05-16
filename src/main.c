#include <complex.h>
#include <math.h>
#include <stdio.h>
#include "agc.h"
#include "interpolator.h"
#include "nco.h"
#include "utils.h"
#include "wavfile.h"

#define SYM_RATE 72000
#define CHUNKSIZE 1024
#define INTERP_FACTOR 8

#define AGC_WINSIZE 1024 * 128
#define AGC_TARGET 120.0

int
main(int argc, char *argv[])
{
	int i, count;
	int complex cur_sample, agc_out;
	Sample *samples;
	Sample *interp;

	if (argc < 2) {
		usage(argv[0]);
	}

	agc_init(AGC_TARGET, AGC_WINSIZE);

	/* Open raw samples file */
	samples = open_samples_file(argv[1]);
	if (!samples) {
		fatal("Couldn't open samples file");
	}

	interp = interp_init(samples, INTERP_FACTOR);
#ifdef __DEBUG
	printf("[main.c] Will interpolate %d sps to %d sps\n",
			samples->samplerate, interp->samplerate);
#endif

	while ((count = interp->read(interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			cur_sample = interp->data[i];
			/* TODO root-raised cosine filter to match the transmitter */
			/* TODO coarse tuning with fourth-power FFT analysis */
			/* TODO AGC */
			agc_out = agc_apply(cur_sample);
			printf("AGC in: %.2f, out: %.2f\n", cabs(cur_sample), cabs(agc_out));
			/* TODO interpolation */
			/* TODO symbol resampling */
			/* TODO fine frequency/phase tuning */
			/* TODO writing to file */

		}

	}

	samples->close(samples);
	return 0;
}

