/**
 * Main demodulator object. This will launch a thread in the background to
 * process the incoming samples, so it'll interpolate and resample them,
 * normalize their amplitude, recover the carrier, and write the decoded symbols
 * to disk */
#ifndef METEOR_DEMOD_H
#define METEOR_DEMOD_H

#include <pthread.h>
#include "agc.h"
#include "pll.h"
#include "source.h"
#include "utils.h"

/* I/O chunk sizes */
#define CHUNKSIZE 32768
#define SYM_CHUNKSIZE 1024

typedef struct {
	Agc *agc;
	Source *interp, *src;
	Costas *cst;
	float sym_period;
	unsigned sym_rate;
	ModScheme mode;
	pthread_t t;

	pthread_mutex_t mutex;
	unsigned bytes_out_count;
	volatile int thr_is_running;
	int8_t out_buf[SYM_CHUNKSIZE];
} Demod;

Demod*        demod_init(Source *src, unsigned interp_factor, unsigned rrc_order, float rrc_alpha, float pll_bw, unsigned sym_rate, ModScheme mode);
void          demod_start(Demod *self, const char *fname);
void          demod_join(Demod *self);

int           demod_status(const Demod *self);
int           demod_is_pll_locked(const Demod *self);
unsigned      demod_get_bytes_out(Demod *self);
uint64_t      demod_get_done(const Demod *self);
uint64_t      demod_get_size(const Demod *self);
float         demod_get_freq(const Demod *self);
float         demod_get_gain(const Demod *self);
const int8_t* demod_get_buf(const Demod *self);

#endif
