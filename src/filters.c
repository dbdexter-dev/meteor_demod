#include <stdarg.h>
#ifdef __DEBUG
#include <stdio.h>
#endif
#include <math.h>
#include "filters.h"
#include "utils.h"

float compute_rrc_coeff(int stage_no, unsigned n_taps, unsigned osf, float alpha);
double compute_lpf_coeff(int stage_no, unsigned order, float wc, float alpha);

/* Create a new filter, a FIR if back_count is 0. Variable length arguments
 * indicate the various coefficients to be used in the filter */
Filter*
filter_new(unsigned fwd_count, unsigned back_count, ...)
{
	Filter *flt;
	int i;
	va_list flt_parm;
	double *fwd_coeff;
	double *back_coeff;

	va_start(flt_parm, back_count);

	flt = safealloc(sizeof(*flt));

	flt->fwd_count = fwd_count;
	flt->back_count = back_count;

	if (fwd_count) {
		fwd_coeff = va_arg(flt_parm, double*);
		flt->fwd_coeff = safealloc(sizeof(*flt->fwd_coeff) * fwd_count);
		flt->mem = safealloc(sizeof(*flt->mem) * (fwd_count + 1));
		for (i=0; i<fwd_count; i++) {
			flt->fwd_coeff[i] = (float)fwd_coeff[i];
		}

		if (back_count) {
			back_coeff = va_arg(flt_parm, double*);
			flt->back_coeff = safealloc(sizeof(*flt->back_coeff) * back_count);
			for (i=0; i<back_count; i++) {
				flt->back_coeff[i] = (float)back_coeff[i];
			}
		}
	}

	va_end(flt_parm);

	for (i=0; i<fwd_count; i++) {
		flt->mem[i] = 0;
	}
	for (i=0; i<back_count; i++) {
		flt->mem[i] = 0;
	}

	return flt;
}

/* Dabisaclly a deep clone of the filter */
Filter*
filter_copy(const Filter *orig)
{
	Filter *ret;
	int i;

	ret = safealloc(sizeof(*ret));

	ret->back_count = orig->back_count;
	ret->fwd_count = orig->fwd_count;

	if(ret->fwd_count) {
		/* Copy feed-forward parameters */
		ret->fwd_coeff = safealloc(sizeof(*ret->fwd_coeff) * ret->fwd_count);
		ret->mem = safealloc(sizeof(*ret->mem) * ret->fwd_count);
		/* Copy the coefficients */
		for (i=0; i<ret->fwd_count; i++) {
			ret->mem[i] = 0;
			ret->fwd_coeff[i] = orig->fwd_coeff[i];
		}
		if (ret->back_count) {
			/* Copy feedback parameters */
			ret->back_coeff = safealloc(sizeof(*ret->back_coeff) * ret->back_count);
			/* Copy the coefficients */
			for (i=0; i<ret->back_count; i++) {
				ret->back_coeff[i] = orig->back_coeff[i];
			}
		}

	}
	return ret;
}

Filter*
filter_butt2(float zeta, float wn, float k)
{
	Filter *flt;
	float t1, t2;
	double fwd_parm[3];
	double back_parm[3];

	t1 = k/(wn*wn);
	t2 = 2*zeta/wn;

	fwd_parm[0] = 4*k/t1*(1.+t2/2);
	fwd_parm[1] = -8*k/(t1);
	fwd_parm[2] = 4*k/t1*(1.+t2/2);

	back_parm[0] = 1.0;
	back_parm[1] = -2.0;
	back_parm[2] = 1.0;

	flt = filter_new(3, 3, fwd_parm, back_parm);
	return flt;
}

/* Generic low-pass filter */
Filter*
filter_lowpass(unsigned order, float wc)
{
	Filter *lpf;
	double *coeffs;
	int i;
	const int alpha = 0.5;      /* Hamming window */

	coeffs = safealloc(sizeof(*coeffs) * (order+1));
	for (i=0; i<=order; i++) {
		coeffs[i] = compute_lpf_coeff(i, order, wc, alpha);
	}

	lpf = filter_new(order+1, 0, coeffs);

	return lpf;
}

/* Root raised cosine filter */
Filter*
filter_rrc(unsigned order, unsigned osf, float alpha)
{
	int i;
	unsigned taps;
	double *coeffs;
	Filter *rrc;

	taps = order*osf+1;

	coeffs = safealloc(sizeof(*coeffs) * taps);
	for (i=0; i<taps; i++) {
		coeffs[i] = compute_rrc_coeff(i, taps, osf, alpha);
	}

	rrc = filter_new(taps, 0, coeffs);
	return rrc;
}


/* Feed a signal through a filter, and output the result */
float complex
filter_fwd(Filter *self, float complex in)
{
	int i;
	float complex out;
	if (!self->fwd_count && ! self->back_count) {
		return 0;
	}

	/* Update the memory nodes */
	for (i=self->fwd_count-1; i>0; i--) {
		self->mem[i] = self->mem[i-1];
	}

	/* Calculate the new mem[0] value */
	for (i=0; i<self->back_count; i--) {
		in -= self->mem[i] * self->back_coeff[i];
	}
	self->mem[0] = in;

	/* Calculate the feed-forward output */
	out = 0;
	for (i=0; i<self->fwd_count; i++) {
		out += self->mem[i] * self->fwd_coeff[i];
	}

	return out;
}

void
filter_free(Filter *self)
{
	if (self->fwd_count) {
		free(self->mem);
		free(self->fwd_coeff);
	}
	if (self->back_count) {
		free(self->mem);
		free(self->back_coeff);
	}
	free(self);
}

float
filter_wn_prewarp(float wn_digital, unsigned samplerate)
{
	float sample_period = 1.0/samplerate;
	return 2.0/sample_period*tan(wn_digital/sample_period);
}

/*Static functions {{{*/
/* Straight-up windowing */
inline double 
compute_lpf_coeff(int stage_no, unsigned order, float wc, float alpha)
{
	double coeff;
	double weight;
	int shifted_time;

	order++;
	shifted_time = (stage_no - (order)/2);
	if (shifted_time == 0) {
		return 1;
	}

	weight = alpha + (1-alpha)*cos(stage_no*M_PI/order);
	coeff = wc/M_PI*sin(wc/M_PI*shifted_time)/(wc/M_PI*shifted_time);
	return coeff * weight;
}

inline float
compute_rrc_coeff(int stage_no, unsigned taps, unsigned osf, float alpha)
{
	float coeff;
	float t;
	float interm;
	unsigned order;

	order = (taps-1)/2;

	t = abs((order - stage_no))/(float)osf;

	if (t==0) {
		return sqrt(M_SQRT2);
	}

	coeff = sin(M_PI*t*(1-alpha)) + 4*alpha*t*cos(M_PI*t*(1+alpha));
	interm = M_PI*t*(1-(4*alpha*t)*(4*alpha*t));
	return coeff / interm;
}

/*}}}*/
