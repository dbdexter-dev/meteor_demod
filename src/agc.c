#ifdef __DEBUG
#include <stdio.h>
#endif
#include "agc.h"
#include "utils.h"

#define AGC_MAX_GAIN 20

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

float complex
agc_apply(Agc *self, float complex sampl)
{
	float rho;

	rho = cabs(sampl);
	self->avg = (self->avg * (self->window_size - 1) + rho) / self->window_size;

	self->gain = self->target_ampl / self->avg;
	if (self->gain > AGC_MAX_GAIN) {
		self->gain = AGC_MAX_GAIN;
	}
	return sampl * self->gain;
}

void
agc_free(Agc *self)
{
	free(self);
}
