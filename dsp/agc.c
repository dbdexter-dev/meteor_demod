#include <math.h>
#include "agc.h"
#include "utils.h"

#define FLOAT_TARGET_MAG 190
#define BIAS_POLE 0.001f
#define GAIN_POLE 0.0001f

static float _float_gain = 1;
static float complex _float_bias = 0;

float complex
agc_apply(float complex sample)
{
	/* Remove DC bias */
	_float_bias = _float_bias*(1-BIAS_POLE) + BIAS_POLE*sample;
	sample -= _float_bias;

	/* Apply AGC */
	sample *= _float_gain;
	_float_gain += GAIN_POLE*(FLOAT_TARGET_MAG - cabsf(sample));
	_float_gain = MAX(0, _float_gain);

	return sample;
}

float
agc_get_gain()
{
	return _float_gain;
}
