#include "agc.h"
#include "utils.h"

static Agc agc;

void
agc_init(float target_ampl, unsigned window_size)
{
	agc.window_size = window_size;
	agc.target_ampl = target_ampl;
	agc.avg = target_ampl;
}

float complex
agc_apply(float complex sampl)
{
	float ampl;

	ampl = cabs(sampl);
	agc.avg = (agc.avg * (agc.window_size - 1) + ampl) / agc.window_size;

	agc.gain = agc.target_ampl / agc.avg;
	return sampl * agc.gain;
}

