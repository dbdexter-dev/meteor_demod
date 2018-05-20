#include <complex.h>
#include <math.h>
#ifdef __DEBUG
#include <stdio.h>
#endif
#include "pll.h"
#include "utils.h"

static float costas_compute_delta(float i_branch, float q_branch);
float freq_clamp(float freq, float max);

/* Initialize a Costas loop for carrier frequency/phase recovery */
Costas*
costas_init(Filter *lpf, Filter *loop_filter, float alpha)
{
	Costas *costas;

	costas = safealloc(sizeof(*costas));

	costas->lpf = filter_copy(lpf);
	costas->loop_filter = filter_copy(loop_filter);
	costas->alpha = alpha;
	costas->nco = nco_init(0);

	return costas;
}

float complex 
costas_resync(Costas *self, float complex samp)
{
	const float damping = 1/M_SQRT2;
	const float bw = 0.015;
	float denom = (1.0 + 2.0*damping*bw + bw*bw);
	float alpha = (4*damping*bw)/denom;
	float beta = (4*bw*bw)/denom;

	float complex interm;
	float complex nco_out;
	float complex retval;
	float error;

	nco_out = nco_step(self->nco);

	/* Mix with LO and low pass filter */
	retval = samp * conj(nco_out);
	interm = retval;
/*	retval = filter_fwd(self->lpf, retval);*/
/*	interm = filter_fwd(self->lpf, retval);*/

	/* Calculate the phase delta */
	error = costas_compute_delta(creal(retval), cimag(retval))/255;
/*	error = filter_fwd(self->lpf, error);*/
	error = freq_clamp(error, 1.0);
/*	error = filter_fwd(self->loop_filter, error);*/
	
#ifdef __DEBUG
	fprintf(stderr, "[pll.c] %f %f\n", alpha, beta);
	fprintf(stderr, "[pll.c] post-filter error: %f\n", error);
	printf("%f %f %f %f %f %f %f %f\n",
	      creal(nco_out), cimag(nco_out),
	      creal(samp), cimag(samp),
	      creal(interm), cimag(interm), 
	      self->nco->freq, 
	      error);
#endif

	/* Apply the phase delta */
	self->nco->phase = fmod(self->nco->phase + alpha*error, 2*M_PI);
	self->nco->freq = freq_clamp(self->nco->freq + beta*error, 1.0);

	return retval;
}

inline float
freq_clamp(float freq, float max_abs)
{
	if (freq > max_abs) {
		return max_abs;
	} else if (freq < -max_abs) {
		return -max_abs;
	}
	return freq;
}

void
costas_free(Costas *cst)
{
	filter_free(cst->lpf);
	filter_free(cst->loop_filter);
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
