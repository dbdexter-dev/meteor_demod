#include <math.h>
#ifdef __DEBUG
#include <stdio.h>
#endif
#include "pll.h"
#include "utils.h"

static float costas_compute_delta(float i_branch, float q_branch);

/* Initialize a Costas loop for carrier frequency/phase recovery */
Costas*
costas_init(float freq, float damping, float bw)
{
	float denom;
	Costas *costas;

	costas = safealloc(sizeof(*costas));

	costas->nco_freq = freq;
	costas->nco_phase = 0;

	denom = (1.0 + 2.0*damping*bw + bw*bw);
	costas->alpha = (4*damping*bw)/denom;
	costas->beta = (4*bw*bw)/denom;

	return costas;
}

float complex 
costas_resync(Costas *self, float complex samp)
{
	float complex nco_out;
	float complex retval;
	float error;

	nco_out = cexp(I*self->nco_phase);

	/* Mix with LO */
	retval = samp * conj(nco_out);

	/* Calculate the phase delta */
	error = costas_compute_delta(creal(retval), cimag(retval))/255;
	error = float_clamp(error, 1.0);
	
	/* Apply the phase delta */
	self->nco_phase = fmod(self->nco_phase + self->nco_freq + self->alpha*error, 2*M_PI);
	self->nco_freq = float_clamp(self->nco_freq + self->beta*error, 1.0);

	return retval;
}


void
costas_free(Costas *cst)
{
	free(cst);
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
