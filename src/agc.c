#include <math.h>
#include "agc.h"
#include "utils.h"

/* AGC default parameters */
#define AGC_WINSIZE 1024*64
#define AGC_TARGET 180
#define AGC_MAX_GAIN 20
#define AGC_BIAS_WINSIZE 1024*1024


/* Initialize an AGC object */
Agc*
agc_init()
{
	Agc *agc;

	agc = safealloc(sizeof(*agc));
	agc->window_size = AGC_WINSIZE;
	agc->target_ampl = AGC_TARGET;
	agc->avg = AGC_TARGET;
	agc->bias = 0;

	return agc;
}

/* Apply the right gain to a sample */
float complex
agc_apply(Agc *self, float complex sample)
{
	float rho;

	self->bias = (self->bias * (AGC_BIAS_WINSIZE-1) + sample) / (AGC_BIAS_WINSIZE);
	sample -= self->bias;

	/* Update the sample magnitude average */
	rho = sqrt(creal(sample)*creal(sample) + cimag(sample)*cimag(sample));
	self->avg = (self->avg * (self->window_size - 1) + rho) / self->window_size;

	self->gain = self->target_ampl / self->avg;
	if (self->gain > AGC_MAX_GAIN) {
		self->gain = AGC_MAX_GAIN;
	}
	return sample * self->gain;
}

/* Free an AGC object */
void
agc_free(Agc *self)
{
	free(self);
}
