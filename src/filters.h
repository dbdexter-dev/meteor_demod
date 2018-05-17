/** Various DSP filters */
#ifndef _METEOR_FILTERS_H
#define _METEOR_FILTERS_H

#include <complex.h>

typedef struct {
	unsigned fwd_count;
	float *fwd_coeff;
	float complex *fwd_mem;

	unsigned back_count;
	float *back_coeff;
	float complex *back_mem;
} Filter;

Filter* filter_new(unsigned fwd_count, unsigned back_count, ...);
Filter* filter_copy(Filter *orig);
float   filter_fwd(Filter *flt, float complex in);
void    filter_free(Filter *flt);
Filter* filter_rrc(unsigned taps, unsigned osf, float beta);

#endif
