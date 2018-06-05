/**
 * Phase-locked loop (actually a Costas loop) defined here. Feed samples to
 * costas_resync, and it'll return the samples resync'd to the reconstructed
 * carrier.
 */
#ifndef _METEOR_PLL_H
#define _METEOR_PLL_H

#include <complex.h>

/* Costas loop default parameters */
#define COSTAS_DAMP 1/M_SQRT2
#define COSTAS_INIT_FREQ 0.001

typedef struct {
	float nco_phase, nco_freq;
	float alpha, beta;
	float damping, bw;
	int locked;
	float moving_avg;
} Costas;

Costas*       costas_init(float bw);
float complex costas_resync(Costas *self, float complex samp);
void          costas_free(Costas *self);

void          costas_recompute_coeffs(Costas *self, float damping, float bw);

#endif
