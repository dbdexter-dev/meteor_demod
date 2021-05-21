#pragma once
#include "dsp/agc.h"
#include "dsp/filter.h"
#include "dsp/pll.h"
#include "dsp/timing.h"

/* Satellite specific settings */
#define RRC_ALPHA 0.6
#define SYM_RATE 72000.0

/* Decoder specific settings */
#define RRC_ORDER 32
#define INTERP_FACTOR 5
#define SYM_BW 0.00005
#define PLL_BW 1

/**
 * Initialize demodulator
 *
 * @param pll_bw carrier estimator PLL bandwidth
 * @param sym_bw symbol timing estimator bandwidth
 * @param samplerate input sample rate
 * @param symrate expected symbol rate
 * @param interp_factor filter interpolation factor
 * @param rrc_order root-raised cosine order
 * @param oqpsk 1 if oqpsk, 0 if qpsk
 */
void demod_init(float pll_bw, float sym_bw, int samplerate, int symrate, int interp_factor, int rrc_order, int oqpsk);

/**
 * Deinitialize demodulator
 */
void demod_deinit();

/**
 * Feed a QPSK sample into the demodulator
 *
 * @param sample pointer to sample to use
 * @return 1 if sample was updated to a demodulateed symbol, 0 otherwise
 */
int demod_qpsk(float complex *sample);

/**
 * Feed a QPSK sample into the demodulator
 *
 * @param sample pointer to sample to use
 * @return 1 if sample was updated to a demodulateed symbol, 0 otherwise
 */
int demod_oqpsk(float complex *sample);
