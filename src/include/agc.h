/**
 * Moving average AGC, there's not much to it really
 */
#ifndef METEOR_AGC_H
#define METEOR_AGC_H

#include <complex.h>

typedef struct {
	unsigned window_size;
	float avg;
	float gain;
	float target_ampl;
	float complex bias;
} Agc;

Agc*          agc_init(void);
float complex agc_apply(Agc *agc, float complex sampl);
void          agc_free(Agc *agc);

#endif
