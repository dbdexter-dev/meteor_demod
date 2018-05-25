#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "demod.h"
#include "interpolator.h"
#include "utils.h"
#include "wavfile.h"

typedef struct {
	Demod *self;
	const char *out_fname;
} ThrArgs;

static void* demod_thr_run(void* args);

Demod*
demod_init(Sample *src, unsigned interp_mult, float pll_bw, unsigned sym_rate)
{
	Demod *ret;

	ret = safealloc(sizeof(*ret));

	ret->src = src;

	/* Initialize the AGC */
	ret->agc = agc_init(AGC_TARGET, AGC_WINSIZE);

	/* Initialize the interpolator, associating raw_samp to it */
	ret->interp = interp_init(src, RRC_ALPHA, RRC_FIR_ORDER, interp_mult, sym_rate);
	/* Discard the first null samples */
	ret->interp->read(ret->interp, RRC_FIR_ORDER*interp_mult);

	/* Initialize costas loop */
	pll_bw = 2*M_PI*pll_bw/sym_rate;
	ret->cst = costas_init(COSTAS_INIT_FREQ, COSTAS_DAMP, pll_bw);

	/* Initialize the early-late timing variables */
	ret->sym_rate = sym_rate;
	ret->sym_period = ret->interp->samplerate/(float)sym_rate;
	pthread_mutex_init(&ret->mutex, NULL);
	ret->bytes_out_count = 0;
	ret->thr_is_running = 1;

	return ret;
}

void
demod_start(Demod *self, const char *fname)
{
	ThrArgs *args;

	args = safealloc(sizeof(*args));

	args->out_fname = fname;
	args->self = self;

	pthread_create(&self->t, NULL, demod_thr_run, (void*)args);
}

int
demod_status(const Demod *self)
{
	return self->thr_is_running;
}

int
demod_is_pll_locked(const Demod *self)
{
	return self->cst->locked;
}

unsigned
demod_get_bytes(Demod *self)
{
	unsigned ret;

	pthread_mutex_lock(&self->mutex);
	ret = self->bytes_out_count;
	pthread_mutex_unlock(&self->mutex);

	return ret;
}

float
demod_get_perc(const Demod *self)
{
	return wav_get_perc(self->src);
}

float
demod_get_freq(const Demod *self)
{
	return self->cst->nco_freq*self->sym_rate/(2*M_PI);
}

/* XXX not thread-safe */
const char*
demod_get_buf(const Demod *self)
{
	return self->out_buf;
}

void
demod_join(Demod *self)
{
	void* retval;

	self->thr_is_running = 0;
	pthread_join(self->t, &retval);

	agc_free(self->agc);
	costas_free(self->cst);
	self->interp->close(self->interp);

	free(self);

}

/* Static functions {{{ */
void*
demod_thr_run(void* x)
{
	FILE *out_fd;
	int i, count, buf_offset;
	float complex early, cur, late;
	float resync_offset, resync_error, resync_period;
	char *out_buf;

	const ThrArgs *args = (ThrArgs*)x;
	Demod *self = args->self;
	out_buf = self->out_buf;

	self->thr_is_running = 1;

	resync_period = self->sym_period;

	if (args->out_fname) {
		if (!(out_fd = fopen(args->out_fname, "w"))) {
			fatal("Could not open file for writing");
		}
	} else {
		out_fd = 0;
	}

	/* Main processing loop */
	buf_offset = 0;
	resync_offset = 0;
	early = 0;
	cur = 0;
	while (self->thr_is_running && (count = self->interp->read(self->interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			/* Symbol resampling */
			late = cur;
			cur = early;
			early = agc_apply(self->agc, self->interp->data[i]);

			resync_offset++;
			if (resync_offset >= resync_period) {
				/* The current sample is in the correct time slot: process it */
				/* Calculate the symbol timing error (early-late algorithm) */
				resync_offset -= resync_period;
				resync_error = (cimag(late) - cimag(early)) * cimag(cur);
				resync_offset += (resync_error/10000*resync_period/100);

				/* Fine frequency/phase tuning */
				cur = costas_resync(self->cst, cur);

				/* Append the new samples to the output buffer */
				out_buf[buf_offset++] = clamp(creal(cur)/2);
				out_buf[buf_offset++] = clamp(cimag(cur)/2);

				/* Write binary stream to file and/or to socket */
				if (buf_offset >= SYM_CHUNKSIZE) {
					fwrite(out_buf, buf_offset, 1, out_fd);
					buf_offset = 0;
				}
				pthread_mutex_lock(&self->mutex);
				self->bytes_out_count += 2;
				pthread_mutex_unlock(&self->mutex);
			}
		}
	}

	/* Write the remaining bytes */
	fwrite(out_buf, buf_offset, 1, out_fd);
	fclose(out_fd);

	free(x);
	self->thr_is_running = 0;
	return NULL;
}
/*}}}*/
