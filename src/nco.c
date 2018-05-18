#include <complex.h>
#include <math.h>
#include "nco.h"
#include "utils.h"

Nco*
nco_init(float freq)
{
	Nco *nco;

	nco = safealloc(sizeof(*nco));
	nco->freq = freq;
	nco->sin_prev = 0;
	nco->cos_prev = 1;

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

	new_out = (nco->cos_prev * cosf(nco->freq) - nco->sin_prev * sinf(nco->freq)) +
	          I*(nco->sin_prev * cosf(nco->freq) + nco->cos_prev * sinf(nco->freq));

	nco->sin_prev = cimag(new_out);
	nco->cos_prev = creal(new_out);

#ifdef __DEBUG
/*	printf("[nco.c] NCO output: (%f %f)\n", creal(new_out), cimag(new_out));*/
/*	printf("%f %f\n", creal(new_out), cimag(new_out));*/
#endif

	return new_out;
}

void
nco_adjust(Nco *nco, float delta_f)
{
#ifdef __DEBUG
/*	printf("[nco.c] Adjusting frequency to %f\n", nco->freq + delta_f);*/
/*	printf("%f\n", (nco->freq+delta_f)*140000);*/
#endif
	nco->freq += delta_f;
}

void
nco_free(Nco *nco)
{
	free(nco);
}
