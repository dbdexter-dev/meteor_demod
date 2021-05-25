#ifndef timing_h
#define timing_h

#include <complex.h>

/**
 * Initialize M&M symbol timing estimator
 *
 * @param sym_freq expected symbol frequency
 * @param bw bandwidth of the loop filter
 */
void timing_init(float sym_freq, float bw);

/**
 * Update symbol timing estimate
 *
 * @param sample sample to update the estimate with
 */
void retime(float complex sample);

/**
 * Advance the internal symbol clock by one sample (not symbol, sample)
 */
int advance_timeslot();
int advance_timeslot_dual();

/**
 * Get the M&M symbol frequency estimate
 *
 * @return frequency
 */
float mm_omega();

#endif
