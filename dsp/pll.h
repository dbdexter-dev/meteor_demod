#ifndef pll_h
#define pll_h
#include <complex.h>

/**
 * Initialize phase locked loop
 *
 * @param bw bandwidth of the loop filter
 */
void  pll_init(float bw, int oqpsk);

/**
 * Get the PLL local oscillator frequency
 *
 * @return frequency
 */
float pll_get_freq();

/**
 * Get current PLL status
 *
 * @return 0 if unlocked, 1 if locked
 */
int pll_get_locked();

/**
 * Check whether the PLL locked at least once in the past
 *
 * @return 0 if it never locked, 1 if it did
 */
int pll_did_lock_once();

/**
 * Update the carrier estimate based on a sample pair
 * (for QPSK, sample = cosample)
 *
 * @param sample in-phase sample
 * @param cosample quadrature sample
 */
void  pll_update_estimate(float complex sample, float complex cosample);

/**
 * Mix a sample with the local oscillator
 *
 * @param sample sample to mix
 * @return PLL output
 */
float complex pll_mix(float complex sample);

#endif
