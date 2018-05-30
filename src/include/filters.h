/**
 * Various DSP filters. filter_new() can be used for both FIR and IIR filters.
 * If back_count == 0, the filter will be FIR, otherwise it'll be IIR. Right now
 * this is used only to build the interpolating root-raised cosine filter
 */
#ifndef _METEOR_FILTERS_H
#define _METEOR_FILTERS_H

#include <complex.h>

typedef struct {
	float complex *mem;

	unsigned fwd_count;
	float *fwd_coeff;

	unsigned back_count;
	float *back_coeff;
} Filter;

Filter*       filter_new(unsigned fwd_count, unsigned back_count, ...);
Filter*       filter_copy(const Filter *orig);

Filter*       filter_rrc(unsigned order, unsigned factor, float osf, float alpha);

float complex filter_fwd(Filter *flt, float complex in);
void          filter_free(Filter *flt);

#endif
