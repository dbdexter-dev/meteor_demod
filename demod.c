#include <complex.h>
#include <math.h>
#include "demod.h"

static Filter _rrc_filter;

void
demod_init(float pll_bw, float sym_bw, int samplerate, int symrate, int interp_factor, int rrc_order, int oqpsk)
{
	const int multiplier = oqpsk ? 1 : 2;   /* OQPSK uses two samples per symbol */

	pll_init(2*M_PI*pll_bw/(multiplier * symrate), oqpsk);
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

			retime(out);                        /* Update symbol clock */
			pll_update_estimate(out, out);      /* Update carrier frequency */

			*sample = out;                      /* Write out symbol */
			ret = 1;
		}
	}

	return ret;
}

int
demod_oqpsk(float complex *sample)
{
	float complex out;
	static float complex inphase;
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
				inphase = pll_mix(out);
				break;
			case 2:
				/* Actual sample */
				out = filter_get(&_rrc_filter, i);  /* Get the filter output */
				out = agc_apply(out);               /* Apply AGC */
				out = pll_mix(out);                 /* Mix with local oscillator */

				retime(out);                        /* Update symbol clock */
				pll_update_estimate(inphase, out);  /* Update carrier frequency */

				*sample = crealf(inphase) + I*cimagf(out);  /* Write out symbol */
				ret = 1;
				break;
			default:
				break;

		}
	}

	return ret;
}
