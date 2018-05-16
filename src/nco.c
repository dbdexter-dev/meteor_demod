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

float complex
nco_step(Nco *nco)
{
	float complex new_out;


	new_out = nco->sin_prev * sin(nco->freq) +
	          I * nco->cos_prev * cos(nco->freq);

	nco->sin_prev = creal(new_out);
	nco->cos_prev = cimag(new_out);

	return new_out;
}

void
nco_adjust(Nco *nco, float delta_f)
{
	nco->freq += delta_f;
}

void
nco_free(Nco *nco)
{
	free(nco);
}
