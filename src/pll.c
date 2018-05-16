#include <complex.h>
#include <math.h>
#include "pll.h"
#include "utils.h"

static float nco_compute_delta(float complex i_branch, float complex q_branch);

Costas*
costas_init(Filter *lpf, Filter *loop_filter, float alpha)
{
	Costas *costas;

	costas = safealloc(sizeof(*costas));
	/* Matched filters for the I and the Q branch */
	costas->lpf_i = filter_copy(lpf);
	costas->lpf_q = filter_copy(lpf);
	costas->loop_filter = filter_copy(loop_filter);

	costas->alpha = alpha;

	costas->nco = nco_init(0);
	return costas;
}

void
costas_resync(Costas *self, float complex *samp)
{
	float complex i_interm, q_interm;
	float complex nco_out;
	float error;

	i_interm = creal(*samp);
	q_interm = cimag(*samp);

	nco_out = nco_step(self->nco);

	/* Mix with LO */
	i_interm = creal(nco_out) * i_interm;
	q_interm = cimag(nco_out) * q_interm;

	/* Low pass filter */
	i_interm = filter_fwd(self->lpf_i, i_interm);
	q_interm = filter_fwd(self->lpf_q, q_interm);

	/* Calculate the phase delta */
	error = nco_compute_delta(i_interm, q_interm);
	
	/* Apply the phase delta */
	error = filter_fwd(self->loop_filter, error);
	nco_adjust(self->nco, error * self->alpha);

	*samp = i_interm + I * q_interm;
}

void
costas_free(Costas *cst)
{
	filter_free(cst->lpf_i);
	filter_free(cst->lpf_q);

	free(cst->loop_filter);
}

/* Static functions {{{ */
/* Compute the delta phase value to use when correcting the NCO frequency */
float
nco_compute_delta(float complex i_branch, float complex q_branch)
{
	int i_sign, q_sign;
	float error;

	i_sign = slice(i_branch);
	q_sign = slice(q_branch);

	error = q_branch * i_sign + i_branch * q_sign;
	return error;
}
/*}}}*/
