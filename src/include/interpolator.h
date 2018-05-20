/** 
 * Interpolator and RRC filter in one object. It turns out, using a RRC you can
 * easily interpolate the incoming samples, and I hope this is the best way to
 * reconstruct a nicer looking signal */
#ifndef _METEOR_INTERPOLATOR_H
#define _METEOR_INTERPOLATOR_H

#include "sample.h"

Sample* interp_init(Sample *src, float alpha, unsigned order, unsigned factor);
int interp_read(Sample *self, size_t count);
int interp_close(Sample *self);

#endif
