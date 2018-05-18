#ifdef __DEBUG
#include <stdio.h>
#endif
#include "agc.h"
#include "utils.h"

#define AGC_MAX_GAIN 200

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
	float rho;

	rho = cabs(sampl);
	agc.avg = (agc.avg * (agc.window_size - 1) + rho) / agc.window_size;

	agc.gain = agc.target_ampl / agc.avg;
	if (agc.gain > AGC_MAX_GAIN) {
		agc.gain = AGC_MAX_GAIN;
	}
	return sampl * agc.gain;
}

