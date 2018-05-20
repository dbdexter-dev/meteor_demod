/**
 * Phase-locked loop (actually a Costas loop) defined here. Feed samples to
 * costas_resync, and it'll return the samples resync'd to the reconstructed
 * carrier.
 */
#ifndef _METEOR_PLL_H
#define _METEOR_PLL_H

#include <complex.h>

typedef struct {
	float nco_phase, nco_freq;
	float alpha, beta;
} Costas;

Costas*       costas_init(float freq, float damping, float bw);
float complex costas_resync(Costas *self, float complex samp);
void          costas_free(Costas *self);

#endif
