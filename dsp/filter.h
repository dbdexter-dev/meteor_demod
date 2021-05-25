#ifndef filter_h
#define filter_h
#include <complex.h>

typedef struct {
	float complex *mem;
	float *coeffs;
	int interp_factor;
	int size;
	int idx;
} Filter;

/**
 * Initialize a FIR filter with coefficients corresponding to a root-raised
 * cosine filter
 *
 * @param flt filter object to initialize
 * @param order order of the filter (e.g. 16 = 16 + 1 + 16 taps)
 * @param osf oversampling factor (samples per symbol)
 * @param alpha filter alpha parameter
 * @param factor interpolation factor
 *
 * @return 0 on success, 1 on failure
 */
int filter_init_rrc(Filter *flt, unsigned order, float osf, float alpha, unsigned factor);

/**
 * Deinitialize a filter object
 *
 * @param flt filter to deinitalize
 */
void filter_deinit(Filter *flt);

/**
 * Feed a sample to a filter object
 *
 * @param flt filter to pass the sample through
 * @param sample sample to feed
 */
void filter_fwd_sample(Filter *flt, float complex sample);

/**
 * Get the output of a filter object
 *
 * @param flt filter to read the sample from
 * @param phase for interpolating filters, the phase to fetch the sample from
 * @return filter output
 */
float complex filter_get(Filter *flt, unsigned phase);

#endif
