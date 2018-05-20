#ifndef _METEOR_NCO_H
#define _METEOR_NCO_H

#include <complex.h>

typedef struct {
	float freq;
	float phase;
} Nco;

Nco*          nco_init(float freq);
float complex nco_step(Nco *nco);
void          nco_adjust(Nco *nco, float delta, float alpha);
void          nco_free(Nco *nco);

#endif
