#include <complex.h>
#include <math.h>
#include "demod.h"

static Filter _rrc_filter;

void
demod_init(float pll_bw, float sym_bw, int samplerate, int symrate, int interp_factor, int rrc_order, int oqpsk, float freq_max)
{
	const int multiplier = oqpsk ? 1 : 2;   /* OQPSK uses two samples per symbol */

	pll_init(2*M_PI*pll_bw/(multiplier * symrate), oqpsk, freq_max);
	timing_init(2*M_PI*symrate/(samplerate*interp_factor), sym_bw/interp_factor);
	filter_init_rrc(&_rrc_filter, rrc_order, (float)samplerate/symrate, RRC_ALPHA, interp_factor);
}

void
demod_deinit()
{
	filter_deinit(&_rrc_filter);
}

int
demod_qpsk(float complex *sample)
{
	float complex out;
	int i, ret;

	filter_fwd_sample(&_rrc_filter, *sample);

	/* Check if this sample is in the correct timeslot */
	ret = 0;
	for (i=0; i<_rrc_filter.interp_factor; i++) {
		if (advance_timeslot()) {
			out = filter_get(&_rrc_filter, i);  /* Get the filter output */
			out = agc_apply(out);               /* Apply AGC */
			out = pll_mix(out);                 /* Mix with local oscillator */

			retime(out);                                    /* Update symbol clock */
			pll_update_estimate(crealf(out), cimagf(out));  /* Update carrier frequency */

			*sample = out;                      /* Write out symbol */
			ret = 1;
		}
	}

	return ret;
}

int
demod_oqpsk(float complex *restrict sample)
{
	float complex out;
	static float inphase;
	float quad;
	int i, ret;

	filter_fwd_sample(&_rrc_filter, *sample);

	/* Check if this sample is in the correct timeslot */
	ret = 0;
	for (i=0; i<_rrc_filter.interp_factor; i++) {
		switch (advance_timeslot_dual()) {
			case 0:
				break;
			case 1:
				/* Intersample */
				out = filter_get(&_rrc_filter, i);
				out = agc_apply(out);
				inphase = pll_mix_i(out);           /* We only care about the I value */
				break;
			case 2:
				/* Actual sample */
				out = filter_get(&_rrc_filter, i);  /* Get the filter output */
				out = agc_apply(out);               /* Apply AGC */
				quad = pll_mix_q(out);              /* We only care about the Q value */

				*sample = inphase + I*quad;

				retime(*sample);                      /* Update symbol clock */
				pll_update_estimate(inphase, quad);  /* Update carrier frequency */
				ret = 1;
				break;
			default:
				break;

		}
	}

	return ret;
}
