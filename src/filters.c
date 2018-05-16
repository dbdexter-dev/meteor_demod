#include <stdarg.h>
#include "filters.h"
#include "utils.h"

/* Create a new filter, a FIR if back_count is 0. Variable length arguments
 * indicate the various coefficients to be used in the filter */
Filter*
filter_new(unsigned fwd_count, unsigned back_count, ...)
{
	Filter *flt;
	int i;
	va_list flt_parm;

	va_start(flt_parm, back_count);

	flt = safealloc(sizeof(*flt));

	flt->fwd_count = fwd_count;
	if (fwd_count) {
		flt->fwd_coeff = safealloc(sizeof(*flt->fwd_coeff) * fwd_count);
		flt->fwd_mem = safealloc(sizeof(*flt->fwd_mem) * fwd_count);
		for (i=0; i<fwd_count; i++) {
			flt->fwd_coeff[i] = va_arg(flt_parm, double);
		}
	}

	flt->back_count = back_count;
	if (back_count) {
		flt->back_coeff = safealloc(sizeof(*flt->back_coeff) * back_count);
		flt->back_mem = safealloc(sizeof(*flt->back_mem) * back_count) ;
		for (i=0; i<back_count; i++) {
			flt->back_coeff[i] = va_arg(flt_parm, double);
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
filter_rrc(float beta)
{
	Filter *rrc = NULL;
	/* TODO actual filter implementation */
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
