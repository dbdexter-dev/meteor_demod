#include <math.h>
#include "pll.h"
#include "utils.h"

#define FREQ_MAX 1.0

static float costas_compute_delta(float i_branch, float q_branch);

/* Initialize a Costas loop for carrier frequency/phase recovery */
Costas*
costas_init(float freq, float damping, float bw)
{
	Costas *costas;

	costas = safealloc(sizeof(*costas));

	costas->nco_freq = freq;
	costas->nco_phase = 0;

	costas_recompute_coeffs(costas, damping, bw);

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

	/* Calculate phase delta */
	error = costas_compute_delta(creal(retval), cimag(retval))/255;
	error = float_clamp(error, 1.0);

	/* Apply phase and frequency corrections */
	self->nco_phase = fmod(self->nco_phase + self->nco_freq + self->alpha*error, 2*M_PI);
	self->nco_freq = float_clamp(self->nco_freq + self->beta*error, FREQ_MAX);

	/* If the PLL goes all wonky, try to bring it back by wrapping the
	 * frequency scale around */
	if (self->nco_freq >= FREQ_MAX) {
		self->nco_freq = -FREQ_MAX;
	} else if (self->nco_freq <= -FREQ_MAX) {
		self->nco_freq = FREQ_MAX;
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
}

/* Static functions {{{ */
/* Compute the delta phase value to use when correcting the NCO frequency */
float
costas_compute_delta(float i_branch, float q_branch)
{
	float error;
	error = q_branch * slice(i_branch) -
	        i_branch * slice(q_branch);
	return error;
}
/*}}}*/
