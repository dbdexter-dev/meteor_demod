#include <math.h>
#include "pll.h"
#include "utils.h"
#include "sincos.h"

#define FREQ_MAX 0.3f
#define ERR_POLE 0.001f
#define M_1_SQRT2 0.7071067811865475f


static void update_estimate(float error);
static void update_alpha_beta(float damp, float bw);
static float compute_error(float complex sample, float complex cosample);
static float lut_tanh(float x);

static float _freq, _phase;
static float _alpha, _beta;
static float _lut_tanh[32];
static float _err;
static int _locked, _locked_once;
static float _bw;
static float _fmax;

void
pll_init(float bw, int oqpsk, float freq_max)
{
	int i;

	/* Use default if freq_max is negative, use 1 if freq_max > 1 */
	if (freq_max < 0) freq_max = FREQ_MAX;
	else freq_max = MIN(1.0f, freq_max);

	_freq = 0;
	_phase = 0;
	_locked = _locked_once = 0;
	_err = 1000;
	_bw = bw;
	_fmax = (oqpsk ? freq_max/2 : freq_max);

	for (i=0; i<(int)LEN(_lut_tanh); i++) {
		_lut_tanh[i] = (float)tanh(i-16);
	}
	update_alpha_beta(M_1_SQRT2, bw);
}

float pll_get_freq() { return _freq; }
int pll_get_locked() { return _locked; }
int pll_did_lock_once() { return _locked_once; }

float complex
pll_mix(float complex sample)
{
	const float sine = fast_sin(-_phase);
	const float cosine = fast_cos(-_phase);
	const float re = crealf(sample);
	const float im = cimagf(sample);

	/* Mix sample */
	sample = (re*cosine - im*sine) + I*(re*sine + im*cosine);
	_phase += _freq;            /* Advance phase based on frequency */
	if (_phase >= 2*M_PI) _phase -= 2*M_PI;

	return sample;
}

void
pll_update_estimate(float complex sample, float complex cosample)
{
	float error;
	error = compute_error(sample, cosample);

	update_estimate(error);
}

/* Static functions {{{ */
static void
update_estimate(float error)
{
	static int updown = 1;
	_phase = fmod(_phase + _alpha*error, 2*M_PI);
	_freq += _beta*error;

	/* Lock detection */
	_err = _err*(1-ERR_POLE) + fabs(error)*ERR_POLE;
	if (_err < 85 && !_locked) {
		_locked = 1;
		_locked_once = 1;
	} else if (_err > 105 && _locked) {
		_locked = 0;
	}

	/* If unlocked, scan the frequency range up and down */
	if (!_locked) _freq += 0.000001 * updown;
	updown = (_freq >= _fmax) ? -1 : (_freq <= -_fmax) ? 1 : updown;
	_freq = MAX(-_fmax, MIN(_fmax, _freq));

}

static void
update_alpha_beta(float damp, float bw)
{
	float denom;

	denom = (1 + 2*damp*bw + bw*bw);
	_alpha = 4*damp*bw/denom;
	_beta = 4*bw*bw/denom;
}

static float
compute_error(float complex sample, float complex cosample)
{
	float err;

	err = lut_tanh(crealf(sample)) * cimagf(cosample) -
		  lut_tanh(cimagf(cosample)) * crealf(sample);

	return err;
}

float
lut_tanh(float val)
{
	if (val > 15) return 1;
	if (val < -16) return -1;
	return _lut_tanh[(int)val+16];
}
/* }}} */
