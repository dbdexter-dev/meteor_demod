/** 
 * Various DSP filters. filter_new() can be used for both FIR and IIR filters.
 * If back_count == 0, the filter will be FIR, otherwise it'll be IIR */
#ifndef _METEOR_FILTERS_H
#define _METEOR_FILTERS_H

#include <complex.h>

typedef struct {
	unsigned fwd_count;
	float *fwd_coeff;

	unsigned back_count;
	float *back_coeff;

	float complex *mem;
} Filter;

Filter*       filter_new(unsigned fwd_count, unsigned back_count, ...);
Filter*       filter_copy(const Filter *orig);

Filter*       filter_rrc(unsigned taps, unsigned osf, float alpha);
Filter*       filter_lowpass(unsigned order, float wc);
Filter*       filter_butt2(float zeta, float wn, float t);

float complex filter_fwd(Filter *flt, float complex in);
void          filter_free(Filter *flt);

float         filter_wn_prewarp(float wn, unsigned samplerate);

#endif
