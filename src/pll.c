#include <stdio.h>
#include <math.h>
#include "pll.h"
#include "tui.h"
#include "utils.h"

static float* _lut_tanh;
inline float lut_tanh(float val);

/* Initialize a Costas loop for carrier frequency/phase recovery */
Costas*
costas_init(float bw, ModScheme mode)
{
	int i;
	Costas *costas;

	costas = safealloc(sizeof(*costas));

	costas->nco_freq = COSTAS_INIT_FREQ;
	costas->nco_phase = 0;

	costas_recompute_coeffs(costas, COSTAS_DAMP, bw);

	costas->damping = COSTAS_DAMP;
	costas->bw = bw;
	costas->mode = mode;

	costas->moving_avg = 1;
	costas->locked = 0;

	_lut_tanh = safealloc(sizeof(float) * 256);
	for (i=0; i<256; i++) {
		_lut_tanh[i] = tanh((i-128));
	}

	return costas;
}

float complex
costas_mix(Costas *self, float complex samp)
{
	float complex nco_out;
	float complex retval;

	nco_out = cexp(-I*self->nco_phase);
	retval = samp * nco_out;
	self->nco_phase += self->nco_freq;

	return retval;
}

void
costas_correct_phase(Costas *self, float err)
{
	err = float_clamp(err, 1.0);
	self->nco_phase = fmod(self->nco_phase + self->alpha*err, 2*M_PI);
	self->nco_freq = self->nco_freq + self->beta*err;

	self->moving_avg = (self->moving_avg * (AVG_WINSIZE-1) + fabs(err))/AVG_WINSIZE;

	/* Detect whether the PLL is locked, and decrease the BW if it is */
	if (self->mode == OQPSK) {
		if (!self->locked && self->moving_avg < 0.85) {
			costas_recompute_coeffs(self, self->damping, self->bw/3);
			self->locked = 1;
		} else if (self->locked && self->moving_avg > 0.9) {
			costas_recompute_coeffs(self, self->damping, self->bw);
			self->locked = 0;
		}
	} else if (self->mode == QPSK) {
		if (!self->locked && self->moving_avg < 0.5) {
			costas_recompute_coeffs(self, self->damping, self->bw/3);
			self->locked = 1;
		} else if (self->locked && self->moving_avg > 0.55) {
			costas_recompute_coeffs(self, self->damping, self->bw);
			self->locked = 0;
		}
	}


	/* Limit frequency to a sensible range */
	if (self->nco_freq <= -FREQ_MAX) {
		self->nco_freq = -FREQ_MAX/2;
	} else if (self->nco_freq >= FREQ_MAX) {
		self->nco_freq = FREQ_MAX/2;
	}
}

/* Compute the alpha and beta coefficients of the Costas loop from damping and
 * bandwidth, and update them in the Costas object */
void
costas_recompute_coeffs(Costas *self, float damping, float bw)
{
	float denom;

	denom = (1.0 + 2.0*damping*bw + bw*bw);
	self->alpha = (4*damping*bw)/denom;
	self->beta = (4*bw*bw)/denom;
}


/* Free the memory associated with the Costas loop object */
void
costas_free(Costas *self)
{
	free(self);
	free(_lut_tanh);
}


/* Compute the delta phase value to use when correcting the NCO frequency (OQPSK) */
float
costas_delta(float complex sample, float complex cosample)
{
	float error;

	error = (lut_tanh(crealf(sample)) * cimagf(sample)) -
	        (lut_tanh(cimagf(cosample)) * crealf(cosample));

	return error/50;
}

/* Static functions {{{ */
float
lut_tanh(float val)
{
	if (val > 127) {
		return 1;
	}
	if (val < -128) {
		return -1;
	}

	return _lut_tanh[(int)val+128];
}
/*}}}*/
