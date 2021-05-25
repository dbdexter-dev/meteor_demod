#include <complex.h>
#include <math.h>
#include "timing.h"
#include "utils.h"

/* freq will be at most +-2**-FREQ_DEV_EXP outside of the range */
#define FREQ_DEV_EXP 12

static void update_estimate(float err);
static void update_alpha_beta(float damp, float bw);
static float mm_err(float prev, float cur);

static float _prev;
static float _phase, _freq;                /* Symbol phase and rate estimate */
static float _freq_max_dev, _center_freq;  /* Max freq deviation and center freq */
static float _alpha, _beta;                /* Proportional and integral loop gain */

void
timing_init(float sym_freq, float bw)
{
	_freq = sym_freq;
	_center_freq = _freq;
	_freq_max_dev = _freq / (1<<FREQ_DEV_EXP);


	update_alpha_beta(1, bw);
}

float mm_omega() {return _freq;}

int
advance_timeslot()
{
	_phase += _freq;

	/* Check if the timeslot is right */
	return _phase >= 2*(float)M_PI;
}

int
advance_timeslot_dual()
{
	static int state = 1;
	int ret;

	/* Phase up */
	_phase += _freq;

	/* Check if the timeslot is right */
	if (_phase >= state * (float)M_PI) {
		ret = state;
		state = (state % 2) + 1;
		return ret;
	}

	return 0;
}

void
retime(float complex sample)
{
	float err;

	/* Compute timing error */
	err = mm_err(_prev, cimagf(sample));
	_prev = cimagf(sample);

	/* Update phase and freq estimate */
	update_estimate(err);
}

/* Static functions {{{ */
static void
update_estimate(float error)
{
	float freq_delta;

	freq_delta = _freq - _center_freq;

	_phase -= 2*M_PI + _alpha*error;
	freq_delta -= _beta*error;

	/* Clip _freq between _freq - _freq_max_dev and _freq + _freq_max_dev */
	freq_delta = MAX(-_freq_max_dev, MIN(_freq_max_dev, freq_delta));
	//freq_delta = ((fabs(freq_delta + _freq_max_dev) - fabs(freq_delta - _freq_max_dev)) / 2.0);
	_freq = _center_freq + freq_delta;
}

static float
mm_err(float prev, float cur)
{
	/* sgn() can be replaced by any decision function used to determine the
	 * symbol value */
	return sgn(prev)*cur - sgn(cur)*prev;
}

static void
update_alpha_beta(float damp, float bw)
{
	float denom;

	denom = (1 + 2*damp*bw + bw*bw);
	_alpha = 4*damp*bw/denom;
	_beta = 4*bw*bw/denom;
}
/* }}} */
