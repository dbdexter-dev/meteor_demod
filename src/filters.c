#include <stdarg.h>
#include <math.h>
#include "filters.h"
#include "utils.h"

inline float compute_rrc_coeff(int stage_no, unsigned n_taps, unsigned osf, float beta);

/* Create a new filter, a FIR if back_count is 0. Variable length arguments
 * indicate the various coefficients to be used in the filter */
Filter*
filter_new(unsigned fwd_count, unsigned back_count, ...)
{
	Filter *flt;
	int i;
	va_list flt_parm;
	double *fwd_coeff;
	double *back_coeff;

	va_start(flt_parm, back_count);

	flt = safealloc(sizeof(*flt));

	flt->fwd_count = fwd_count;
	if (fwd_count) {
		fwd_coeff = va_arg(flt_parm, double*);
		flt->fwd_coeff = safealloc(sizeof(*flt->fwd_coeff) * fwd_count);
		flt->fwd_mem = safealloc(sizeof(*flt->fwd_mem) * fwd_count);
		for (i=0; i<fwd_count; i++) {
			flt->fwd_coeff[i] = fwd_coeff[i];
		}
	}

	flt->back_count = back_count;
	if (back_count) {
		back_coeff = va_arg(flt_parm, double*);
		flt->back_coeff = safealloc(sizeof(*flt->back_coeff) * back_count);
		flt->back_mem = safealloc(sizeof(*flt->back_mem) * back_count) ;
		for (i=0; i<back_count; i++) {
			flt->back_coeff[i] = back_coeff[i];
		}
	}

	va_end(flt_parm);

	for (i=0; i<fwd_count; i++) {
		flt->fwd_mem[i] = 0;
	}
	for (i=0; i<back_count; i++) {
		flt->back_mem[i] = 0;
	}

	return flt;
}

Filter*
filter_copy(Filter *orig)
{
	Filter *ret;
	int i;

	ret = safealloc(sizeof(*ret));

	ret->back_count = orig->back_count;
	ret->fwd_count = orig->fwd_count;

	if (ret->back_count) {
		ret->back_coeff = safealloc(sizeof(*ret->back_coeff) * ret->back_count);
		ret->back_mem = safealloc(sizeof(*ret->back_mem) * ret->back_count);

		for (i=0; i<orig->back_count; i++) {
			ret->back_coeff[i] = orig->back_coeff[i];
			ret->back_mem[i] = 0;
		}
	}

	if(ret->fwd_count) {
		ret->fwd_coeff = safealloc(sizeof(*ret->fwd_coeff) * ret->fwd_count);
		ret->fwd_mem = safealloc(sizeof(*ret->fwd_mem) * ret->fwd_count);
		for (i=0; i<orig->fwd_count; i++) {
			ret->fwd_coeff[i] = orig->fwd_coeff[i];
			ret->fwd_mem[i] = 0;
		}
	}
	return ret;
}

/* Root raised cosine filter */
Filter*
filter_rrc(unsigned order, unsigned osf, float beta)
{
	int i;
	unsigned taps;
	float *coeffs;
	Filter *rrc;

	taps = 2*order+1;

	coeffs = safealloc(sizeof(*coeffs) * (taps));
	for (i=0; i<taps; i++) {
		coeffs[i] = compute_rrc_coeff(i, taps, osf, beta);
	}

	rrc = filter_new(2*taps+1, 0, coeffs);
	return rrc;
}


/* Feed a signad throguh a filter, and output the result */
float
filter_fwd(Filter *self, float complex in)
{
	int i;
	float complex out;

	if (self->fwd_count) {
		/* Update the memory nodes */
		for (i=self->fwd_count; i>0; i--) {
			self->fwd_mem[i] = self->fwd_mem[i-1];
		}
		self->fwd_mem[0] = in;

		/* Calculate the feed-forward portion output */
		out = 0;
		for (i=0; i<self->fwd_count; i++) {
			out += self->fwd_mem[i] * self->fwd_coeff[i];
		}
	} else {
		out = in;
	}

	if (self->back_count) {
		/* Calculate the feedback portion output */
		for (i=0; i<self->back_count; i++) {
			out -= self->back_mem[i] * self->back_coeff[i];
		}

		/* Update the memory nodes */
		for (i=self->back_count; i>0; i--) {
			self->back_mem[i] = self->back_mem[i-1];
		}

		self->back_mem[0] = out;
	}

	return out;
}

void
filter_free(Filter *self)
{
	if (self->fwd_count) {
		free(self->fwd_mem);
		free(self->fwd_coeff);
	}
	if (self->back_count) {
		free(self->back_mem);
		free(self->back_coeff);
	}
	free(self);
}

/*Static functions {{{*/
inline float
compute_rrc_coeff(int stage_no, unsigned n_taps, unsigned osf, float beta)
{
	float coeff;
	float t;

	t = (n_taps - stage_no)/osf;

	coeff = sin(M_PI*t*(1-beta)) + 4*beta*t*cos(M_PI*t*(1+beta));
	coeff /= M_PI*t*(1-(4*beta*t)*(4*beta*t));

	return coeff;
}
/*}}}*/
