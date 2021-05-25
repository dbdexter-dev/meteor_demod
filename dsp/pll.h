#ifndef pll_h
#define pll_h
#include <complex.h>

/**
 * Initialize phase locked loop
 *
 * @param bw bandwidth of the loop filter
 * @param oqpsk 0 if QPSK modulation, 1 if OQPSK
 * @param freq_max maximum carrier deviation, in (1/symbol_rate) rad/s
 *        e.g. freq_max=0.3 -> +-3.5kHz @72ksym/s, +-3.8kHz @80ksym/s
 */
void  pll_init(float bw, int oqpsk, float freq_max);

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
 * @param i in-phase sample
 * @param q quadrature sample
 */
void  pll_update_estimate(float i, float q);

/**
 * Mix a sample with the local oscillator
 *
 * @param sample sample to mix
 * @return PLL output
 */
float complex pll_mix(float complex sample);

/**
 * Partially mix a sample with the local oscillator, returning only the I branch
 * or the Q branch depending on the function
 *
 * @param sample sample to mix
 * @return PLL output (I branch only/Q branch only)
 */
float pll_mix_i(float complex sample);
float pll_mix_q(float complex sample);

#endif
