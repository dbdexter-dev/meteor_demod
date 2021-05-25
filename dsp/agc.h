#ifndef agc_h
#define agc_h
#include <complex.h>

/**
 * Automatic gain control loop
 *
 * @param sample sample to rescale
 * @return scaled sample
 */
float complex agc_apply(float complex sample);

/**
 * Get the current gain of the AGC
 *
 * @return gain
 */
float agc_get_gain();

#endif
