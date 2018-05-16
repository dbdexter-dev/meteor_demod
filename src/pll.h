#ifndef _METEOR_PLL_H
#define _METEOR_PLL_H

#include "filters.h"
#include "nco.h"

typedef struct {
	Nco *nco;
	Filter *lpf_i, *lpf_q;
	Filter *loop_filter;
	float alpha;
} Costas;

Costas* costas_init(Filter *lpf, Filter *loop_filter, float alpha);
void    costas_resync(Costas *self, float complex *samp);

#endif
