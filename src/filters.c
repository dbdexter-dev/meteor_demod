#include <stdarg.h>
#include <math.h>
#include "filters.h"
#include "utils.h"

float compute_rrc_coeff(int stage_no, unsigned n_taps, float osf, float alpha);

/* Create a new filter, a FIR if back_count is 0, an IIR filter otherwise.
 * Variable length arguments are two ptrs to doubles, holding the coefficients
 * to use in the filter */
Filter*
filter_new(unsigned fwd_count, unsigned back_count, ...)
{
	Filter *flt;
	int i;
	va_list flt_parm;
	double *fwd_coeff;
	double *back_coeff;

	flt = safealloc(sizeof(*flt));

	flt->fwd_count = fwd_count;
	flt->back_count = back_count;

	va_start(flt_parm, back_count);
	if (fwd_count) {
		/* Initialize the filter memory nodes and forward coefficients */
		fwd_coeff = va_arg(flt_parm, double*);
		flt->fwd_coeff = safealloc(sizeof(*flt->fwd_coeff) * fwd_count);
		flt->mem = safealloc(sizeof(*flt->mem) * fwd_count);
		for (i=0; i<fwd_count; i++) {
			flt->mem[i] = 0;
			flt->fwd_coeff[i] = (float)fwd_coeff[i];
		}

		if (back_count) {
			/* Initialize the feedback coefficients */
			back_coeff = va_arg(flt_parm, double*);
			flt->back_coeff = safealloc(sizeof(*flt->back_coeff) * back_count);
			for (i=0; i<back_count; i++) {
				flt->back_coeff[i] = (float)back_coeff[i];
			}
		}
	}
	va_end(flt_parm);

	return flt;
}

/* Basically a deep clone of the filter */
Filter*
filter_copy(const Filter *orig)
{
	Filter *ret;
	int i;

	ret = safealloc(sizeof(*ret));

	ret->back_count = orig->back_count;
	ret->fwd_count = orig->fwd_count;

	if(ret->fwd_count) {
		/* Copy feed-forward parameters and initialize the memory */
		ret->fwd_coeff = safealloc(sizeof(*ret->fwd_coeff) * ret->fwd_count);
		ret->mem = safealloc(sizeof(*ret->mem) * ret->fwd_count);
		for (i=0; i<ret->fwd_count; i++) {
			ret->mem[i] = 0;
			ret->fwd_coeff[i] = orig->fwd_coeff[i];
		}
		if (ret->back_count) {
			/* Copy feedback parameters */
			ret->back_coeff = safealloc(sizeof(*ret->back_coeff) * ret->back_count);
			for (i=0; i<ret->back_count; i++) {
				ret->back_coeff[i] = orig->back_coeff[i];
			}
		}

	}
	return ret;
}

/* Create a RRC (root raised cosine) filter */
Filter*
filter_rrc(unsigned order, unsigned factor, float osf, float alpha)
{
	int i;
	unsigned taps;
	double *coeffs;
	Filter *rrc;

	taps = order*factor+1;

	coeffs = safealloc(sizeof(*coeffs) * taps);
	for (i=0; i<taps; i++) {
		coeffs[i] = compute_rrc_coeff(i, taps, factor*osf, alpha);
	}

	rrc = filter_new(taps, 0, coeffs);
	free(coeffs);

	return rrc;
}


/* Feed a signal through a filter, and output the result */
float complex
filter_fwd(Filter *self, float complex in)
{
	int i;
	float complex out;

	if (!self->fwd_count && !self->back_count) {
		return in;
	}

	/* Update the memory nodes */
	for (i=self->fwd_count-1; i>0; i--) {
		self->mem[i] = self->mem[i-1];
	}

	/* Calculate the new mem[0] value through the feedback coefficients */
	for (i=1; i<self->back_count; i++) {
		in -= self->mem[i] * self->back_coeff[i];
	}
	self->mem[0] = in;

	/* Calculate the feed-forward output */
	out = 0;
	for (i=0; i<self->fwd_count; i++) {
		out += self->mem[i] * self->fwd_coeff[i];
	}

	return out;
}

/* Free a filter object */
void
filter_free(Filter *self)
{
	if (self->fwd_count) {
		free(self->mem);
		free(self->fwd_coeff);
	}
	if (self->back_count) {
		free(self->mem);
		free(self->back_coeff);
	}
	free(self);
}

/*Static functions {{{*/
/* Variable alpha RRC filter coefficients */
/* Taken from https://www.michael-joost.de/rrcfilter.pdf */
inline float
compute_rrc_coeff(int stage_no, unsigned taps, float osf, float alpha)
{
	float coeff;
	float t;
	float interm;
	unsigned order;

	order = (taps-1)/2;

	t = abs(order - stage_no)/(float)osf;

	if (t==0) {
		return sqrt(M_SQRT2);
	}

	coeff = sin(M_PI*t*(1-alpha)) + 4*alpha*t*cos(M_PI*t*(1+alpha));
	interm = M_PI*t*(1-(4*alpha*t)*(4*alpha*t));
	return coeff / interm;
}
/*}}}*/
