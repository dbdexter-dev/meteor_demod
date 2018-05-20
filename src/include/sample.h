#ifndef _METEOR_SAMPLE_H
#define _METEOR_SAMPLE_H

#include <stdlib.h>
#include <complex.h>

typedef struct sample {
	size_t count;
	unsigned bps;       /* Bytes per sample */
	unsigned samplerate;
	float complex *data;
	int (*read)(struct sample *, size_t);
	int (*close)(struct sample *);
	void *_backend;     /* Opaque pointer to stuff used by read() and close() */
} Sample;

#endif
