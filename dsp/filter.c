#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include "filter.h"
#include "utils.h"

float rrc_coeff(int stage_no, unsigned n_taps, float osf, float alpha);

int
filter_init_rrc(Filter *flt, unsigned order, float osf, float alpha, unsigned factor)
{
	const unsigned taps = order*2+1;
	unsigned i, j;

	if (!(flt->coeffs = malloc(sizeof(*flt->coeffs) * taps * factor))) return 1;
	if (!(flt->mem = calloc(taps, sizeof(*flt->mem)))) return 1;

	for (j=0; j<(unsigned)factor; j++) {
		for (i=0; i<taps; i++) {
			flt->coeffs[j*taps + i] = rrc_coeff(i*factor + j, taps*factor, osf*factor, alpha);
		}
	}

	flt->size = taps;
	flt->interp_factor = factor;

	return 0;
}

void
filter_deinit(Filter *flt)
{
	if (flt->mem) { free(flt->mem); flt->mem=NULL; }
	if (flt->coeffs) { free(flt->coeffs); flt->coeffs=NULL; }
	flt->size = 0;
}

void
filter_fwd_sample(Filter *flt, float complex sample)
{
	flt->mem[flt->idx++] = sample;
	flt->idx %= flt->size;
}

float complex
filter_get(Filter *flt, unsigned phase)
{
	float complex result;
	int i, j;

	result = 0;
	j = (flt->interp_factor - phase - 1)*flt->size;

	/* Chunk 1: from current position to end */
	for (i=flt->idx; i<flt->size; i++, j++) {
		result += flt->mem[i] * flt->coeffs[j];
	}

	/* Chunk 2: from start to current position - 1 */
	for (i=0; i<flt->idx; i++, j++) {
		result += flt->mem[i] * flt->coeffs[j];
	}

	return result;
}

/*Static functions {{{*/
/* Variable alpha RRC filter coefficients */
/* Taken from https://www.michael-joost.de/rrcfilter.pdf */
float
rrc_coeff(int stage_no, unsigned taps, float osf, float alpha)
{
	const float norm = 2.0/5.0;
	float coeff;
	float t;
	float interm;
	int order;

	order = (taps - 1)/2;

	/* Handle the 0/0 case */
	if (order == stage_no) {
		return norm * (1-alpha+4*alpha/M_PI);
	}

	t = abs(order - stage_no)/osf;
	coeff = sinf(M_PI*t*(1-alpha)) + 4*alpha*t*cosf(M_PI*t*(1+alpha));
	interm = M_PI*t*(1-(4*alpha*t)*(4*alpha*t));

	/* Hamming window */
	coeff *= 0.42 - 0.5*cosf(2*M_PI*stage_no/(taps-1)) + 0.08*cosf(4*M_PI*stage_no/(taps-1));

	return  coeff / interm * norm;
}
/*}}}*/
