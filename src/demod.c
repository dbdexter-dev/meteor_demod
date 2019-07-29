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
	FILE *fd;
} ThrArgs;

static void* demod_thr_qpsk(void* args);
static void* demod_thr_oqpsk(void* args);

Demod*
demod_init(Source *src, unsigned interp_mult, unsigned rrc_order, float rrc_alpha, float pll_bw, unsigned sym_rate, ModScheme mode)
{
	Demod *ret;

	ret = safealloc(sizeof(*ret));

	ret->src = src;

	/* Initialize the AGC */
	ret->agc = agc_init();

	/* Initialize the interpolator, associating raw_samp to it */
	ret->interp = interp_init(src, rrc_alpha, rrc_order, interp_mult, sym_rate);
	/* Discard the first null samples */
	ret->interp->read(ret->interp, rrc_order*interp_mult);

	/* Initialize Costas loop */
	pll_bw = 2*M_PI*pll_bw/sym_rate;
	ret->cst = costas_init(pll_bw, mode);
	ret->mode = mode;

	/* Initialize the timing recovery variables */
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


	args->self = self;
	if (!(args->fd = fopen(fname, "wb"))) {
		fatal("Could not create/open output file");
		free(args);
		/* Not reached */
		return;
	}

	if (self->mode == QPSK) {
		pthread_create(&self->t, NULL, demod_thr_qpsk, (void*)args);
	} else if (self->mode == OQPSK) {
		pthread_create(&self->t, NULL, demod_thr_oqpsk, (void*)args);
	}
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
demod_get_bytes_out(Demod *self)
{
	unsigned ret;

	pthread_mutex_lock(&self->mutex);
	ret = self->bytes_out_count;
	pthread_mutex_unlock(&self->mutex);

	return ret;
}

uint64_t
demod_get_done(const Demod *self)
{
	return self->src->done(self->src);
}

uint64_t
demod_get_size(const Demod *self)
{
	return self->src->size(self->src);
}

float
demod_get_freq(const Demod *self)
{
	if (self->mode == OQPSK) {
		return 2*self->cst->nco_freq*self->sym_rate/(2*M_PI);
	}

	return self->cst->nco_freq*self->sym_rate/(2*M_PI);
}

float
demod_get_gain(const Demod *self)
{
	return self->agc->gain;
}

/* XXX not thread-safe */
const int8_t*
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
	pthread_mutex_destroy(&self->mutex);

	agc_free(self->agc);
	costas_free(self->cst);
	self->interp->close(self->interp);

	free(self);
}

/* Static functions {{{ */
/* Append a symbol to a file */
void
demod_write_symbol(Demod *self, FILE *out_fd, float complex sym, int nobuffer)
{
	static int offset = 0;
	int8_t *const out_buf = self->out_buf;

	out_buf[offset++] = clamp(crealf(sym)/2);
	out_buf[offset++] = clamp(cimagf(sym)/2);

	if (offset >= SYM_CHUNKSIZE - 1 || nobuffer) {
		fwrite(out_buf, offset, 1, out_fd);
		offset = 0;
	}

	pthread_mutex_lock(&self->mutex);
	self->bytes_out_count += 2;
	pthread_mutex_unlock(&self->mutex);
}

void*
demod_thr_qpsk(void* x)
{
	const ThrArgs *args = (ThrArgs*)x;
	int i, count;
	float complex tmp;
	float complex before, mid, cur;
	float resync_offset, resync_error, resync_period;

	Demod *self = args->self;
	resync_period = self->sym_period;

	resync_offset = 0;
	before = 0;
	mid = 0;
	cur = 0;

	/* Main processing loop */
	while (self->thr_is_running && (count = self->interp->read(self->interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			tmp = self->interp->data[i];

			/* symbol timing recovery (Gardner) */
			if (resync_offset >= resync_period/2 && resync_offset < resync_period/2+1) {
				mid = agc_apply(self->agc, tmp);
			} else if (resync_offset >= resync_period) {
				cur = agc_apply(self->agc, tmp);
				resync_offset -= resync_period;
				resync_error = (cimagf(cur) - cimagf(before)) * cimagf(mid);
				resync_offset += (resync_error*resync_period/2000000.0);
				before = cur;

				/* Costas loop frequency/phase tuning */
				cur = costas_mix(self->cst, cur);
				costas_correct_phase(self->cst, costas_delta(cur, cur));

				/* Append the new samples to the output file */
				demod_write_symbol(self, args->fd, cur, 0);
			}
			resync_offset++;
		}
	}

	/* Write the remaining bytes */
	demod_write_symbol(self, args->fd, 0, 1);
	fclose(args->fd);

	free(x);
	self->thr_is_running = 0;
	return NULL;
}

void*
demod_thr_oqpsk(void *x)
{
	int i, count;
	float complex tmp, inphase, quad;
	float complex before, mid, cur;
	float prev_i;
	float resync_error, resync_offset;
	const ThrArgs *args = (ThrArgs*)x;
	Demod *const self = args->self;

	resync_offset = 0;
	before = 0;
	prev_i = 0;

	/* Main processing loop */
	while (self->thr_is_running && (count = self->interp->read(self->interp, CHUNKSIZE))) {
		for (i=0; i<count; i++) {
			tmp = self->interp->data[i];

			if (resync_offset >= self->sym_period/2 && resync_offset < self->sym_period/2+1) {
				inphase = costas_mix(self->cst, agc_apply(self->agc, tmp));
				mid = prev_i + I * cimagf(inphase);
				prev_i = crealf(inphase);

			} else if (resync_offset >= self->sym_period) {
				/* Symbol timing recovery (Gardner) */
				quad = costas_mix(self->cst, agc_apply(self->agc, tmp));
				cur = prev_i + I * cimagf(quad);
				prev_i = crealf(quad);

				resync_offset -= self->sym_period;
				resync_error = (cimagf(quad) - cimagf(before)) * cimagf(mid);
				resync_offset += (resync_error*self->sym_period/2000000.0);
				before = cur;

				/* Carrier tracking */
				costas_correct_phase(self->cst, costas_delta(inphase, quad));
				tmp = crealf(inphase) + I * cimagf(quad);

				demod_write_symbol(self, args->fd, tmp, 0);
			}
			resync_offset++;
		}
	}

	demod_write_symbol(self, args->fd, 0, 1);
	fclose(args->fd);

	free(x);
	self->thr_is_running = 0;
	return NULL;
}
/*}}}*/
