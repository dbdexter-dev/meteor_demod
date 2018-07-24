#include <stdio.h>
#include <math.h>
#include "pll.h"
#include "tui.h"
#include "utils.h"

#define FREQ_MAX 0.8
#define AVG_WINSIZE 40000

static float costas_compute_delta(float i_branch, float q_branch);
static float* _lut_tanh;
inline float lut_tanh(float val);

/* Initialize a Costas loop for carrier frequency/phase recovery */
Costas*
costas_init(float bw)
{
	int i;
	Costas *costas;

	costas = safealloc(sizeof(*costas));

	costas->nco_freq = COSTAS_INIT_FREQ;
	costas->nco_phase = 0;

	costas_recompute_coeffs(costas, COSTAS_DAMP, bw);

	costas->damping = COSTAS_DAMP;
	costas->bw = bw;

	costas->moving_avg = 1;
	costas->locked = 0;

	_lut_tanh = safealloc(sizeof(float) * 256);
	for (i=0; i<256; i++) {
		_lut_tanh[i] = tanh((i-128));
	}


	return costas;
}

/* Demodulate a sample with the reconstructed carrier, and update the carrier
 * itself based on how far the sample is to the closest constellation point */
float complex
costas_resync(Costas *self, float complex samp)
{
	float complex nco_out;
	float complex retval;
	float error;

	/* Convert the phase into the sine and cosine components of the LO */
	nco_out = cexp(-I*self->nco_phase);

	/* Mix sample with LO */
	retval = samp * nco_out;

	/* Calculate phase delta and updothe the running average */
	error = costas_compute_delta(crealf(retval), cimagf(retval))/255.0;
	self->moving_avg = (self->moving_avg * (AVG_WINSIZE-1) + fabs(error))/AVG_WINSIZE;
	error = float_clamp(error, 1.0);

	/* Apply phase and frequency corrections, and advance the phase */
	self->nco_phase = fmod(self->nco_phase + self->nco_freq + self->alpha*error, 2*M_PI);
	self->nco_freq = self->nco_freq + self->beta*error;

	if (self->nco_freq <= -FREQ_MAX) {
		self->nco_freq = -FREQ_MAX/2;
	} else if (self->nco_freq >= FREQ_MAX) {
		self->nco_freq = FREQ_MAX/2;
	}

	/* Detect whether the PLL is locked, and decrease the BW if it is */
	if (!self->locked && self->moving_avg < 0.3) {
		costas_recompute_coeffs(self, self->damping, self->bw/2);
		self->locked = 1;
	} else if (self->locked && self->moving_avg > 0.35) {
		costas_recompute_coeffs(self, self->damping, self->bw);
		self->locked = 0;
	}



	return retval;
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

/* Static functions {{{ */
/* Compute the delta phase value to use when correcting the NCO frequency */
float
costas_compute_delta(float i_branch, float q_branch)
{
	float error;
	error = q_branch * lut_tanh(i_branch) -
	        i_branch * lut_tanh(q_branch);
	return error;
}

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
