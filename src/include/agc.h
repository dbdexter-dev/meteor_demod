/**
 * Moving average AGC, there's not much to it really
 */
#ifndef _METEOR_AGC_H
#define _METEOR_AGC_H

#include <complex.h>

#define AGC_MAX_GAIN 20

typedef struct {
	unsigned window_size;
	float avg;
	float gain;
	float target_ampl;
} Agc;

Agc*          agc_init(float target_ampl, unsigned window_size);
float complex agc_apply(Agc *agc, float complex sampl);
void          agc_free(Agc *agc);

#endif
