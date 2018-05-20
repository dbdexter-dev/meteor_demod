#include <complex.h>
#include <math.h>
#ifdef __DEBUG
#include <stdio.h>
#endif
#include "nco.h"
#include "utils.h"


Nco*
nco_init(float freq)
{
	Nco *nco;

	nco = safealloc(sizeof(*nco));
	nco->freq = freq;
	nco->phase = 0;

	return nco;
}

/* CORDIC algorithm for sine-cosine generation */
float complex
nco_step(Nco *nco)
{
	float complex new_out;
#ifdef __DEBUG
/*	printf("[nco.c] NCO before: (%f %f)\n", nco->sin_prev, nco->cos_prev);*/
/*	printf("%f %f\n", creal(new_out), cimag(new_out));*/
#endif

	nco->phase = fmod(nco->phase + nco->freq, 2*M_PI);
	new_out = cexp(I*nco->phase);

#ifdef __DEBUG
/*	printf("[nco.c] NCO output: (%f %f)\n", creal(new_out), cimag(new_out));*/
/*	printf("%f %f\n", creal(new_out), cimag(new_out));*/
#endif

	return new_out;
}

void
nco_adjust(Nco *nco, float delta, float alpha)
{
#ifdef __DEBUG
/*	printf("%f\n", (nco->freq + delta_f));*/
#endif
	nco->phase = fmod(nco->phase + delta * alpha, 2*M_PI);
	nco->freq += delta * alpha * alpha * .5;
	nco->freq = freq_clamp(nco->freq, M_PI);
}

void
nco_free(Nco *nco)
{
	free(nco);
}
