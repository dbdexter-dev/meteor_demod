#ifndef _METEOR_DEMOD_H
#define _METEOR_DEMOD_H

#include <pthread.h>
#include "agc.h"
#include "pll.h"
#include "sample.h"

typedef struct {
	Agc *agc;
	Sample *interp, *src;
	Costas *cst;
	float sym_period;
	unsigned sym_rate;
	pthread_t t;

	pthread_mutex_t mutex;
	unsigned bytes_out_count;
	int thr_is_running;
} Demod;

Demod*   demod_init(Sample *src, unsigned interp_factor, float pll_bw, unsigned sym_rate);
void     demod_start(Demod *self, int net_port, char *fname);
void     demod_join(Demod *self);

int      demod_status(Demod *self);
unsigned demod_get_bytes(Demod *self);
float    demod_get_perc(Demod *self);
float    demod_get_freq(Demod *self);

#endif
