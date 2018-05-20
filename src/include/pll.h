#ifndef _METEOR_PLL_H
#define _METEOR_PLL_H

#include <complex.h>

typedef struct {
	float nco_phase, nco_freq;
	float alpha, beta;
} Costas;

Costas*       costas_init(float freq, float damping, float bw);
float complex costas_resync(Costas *self, float complex samp);

#endif
