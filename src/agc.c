#include <math.h>
#include "agc.h"
#include "utils.h"

/* Initialize an AGC object */
Agc*
agc_init(float target_ampl, unsigned window_size)
{
	Agc *agc;

	agc = safealloc(sizeof(*agc));
	agc->window_size = window_size;
	agc->target_ampl = target_ampl;
	agc->avg = target_ampl;

	return agc;
}

/* Apply the right gain to a sample */
float complex
agc_apply(Agc *self, float complex sample)
{
	float rho;

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
