/**
 * Interpolator and RRC filter in one object. It turns out, using a RRC you can
 * easily interpolate the incoming samples, and hopefully this is the best way
 * to reconstruct a nicer looking signal
 */
#ifndef METEOR_INTERPOLATOR_H
#define METEOR_INTERPOLATOR_H

#include "source.h"

Source* interp_init(Source *src, float alpha, unsigned order, unsigned factor, int sym_rate);

#endif
