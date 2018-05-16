#include <stdlib.h>
#ifdef __DEBUG
#include <stdio.h>
#endif
#include "interpolator.h"
#include "utils.h"

typedef struct {
	Sample *src;
	unsigned factor;
	int interpd_offset;
} InterpStatus;

static int interp_read(Sample *self, size_t count);
static int interp_close(Sample *self);

Sample*
interp_init(Sample* src, unsigned factor)
{
	Sample *interp;
	InterpStatus *status;

	interp = safealloc(sizeof(*interp));

	interp->count = 0;
	interp->samplerate = src->samplerate * factor;
	interp->data = NULL;
	interp->bps = sizeof(*src->data);
	interp->read = interp_read;
	interp->close = interp_close;

	interp->_backend = safealloc(sizeof(InterpStatus));
	status = (InterpStatus*) interp->_backend;

	status->src = src;
	status->factor = factor;
	status->interpd_offset  = 0;

	return interp;
}

/* Wrapper to interpolate the source data and provide a transparent translation
 * layer between the two blocks. TODO: the first sample is actually invalid
 * because it comes from a previos read, so there's a duplicate sample in two
 * successive calls to interp_read */
static int
interp_read(Sample *self, size_t count)
{
	InterpStatus *status;
	int i, j;
	int factor;
	Sample *src;
	int interpd_offset;
	size_t true_samp_count;
	float complex last_interpd;
	float complex last_sampled;
	float complex avg;

	/* Retrieve the backend info */
	status = (InterpStatus*)self->_backend;
	factor = status->factor;
	src = status->src;
	interpd_offset = status->interpd_offset;

	/* Save the last true sample */
	if (src->data) {
		last_sampled = src->data[src->count-1];
	} else {
		last_sampled = 0;
	}

	/* If there's some data from a previous interpolation, move it to the
	 * beginning of the new data */
	if (!self->data) {
		self->data = safealloc(sizeof(*self->data) * count);
		last_interpd = 0;
	} else {
		/* Save the last interpolated sample */
		last_interpd = self->data[self->count-1];
		if (self->count < count) {
			self->data = realloc(self->data, sizeof(*self->data) * count + 2);
		}
	}

	self->count = count;
	true_samp_count = count / factor;

	/* Read the true samples from the associated source */
	src->read(src, true_samp_count);

	/* Correct for continuous interpolation */
	if (interpd_offset) {
		self->data[0] = last_interpd;
		self->data[factor - interpd_offset] = last_sampled;
		i = 1;
	} else {
		self->data[0] = last_sampled;
		i = 0;
	}
	
	/* Populate data with the real samples */
	for (; i<true_samp_count; i++) {
		self->data[i*factor + (factor - interpd_offset)] = src->data[i];
#ifdef __DEBUG
		printf("[interpolator.c] Read (%f %f)\n", creal(src->data[i]), cimag(src->data[i]));
#endif
	}

	/* Linearly interpolate */
	for (i=0; i<true_samp_count; i++) {
		for (j=1; j<factor; j++) {
			avg = (self->data[i*factor] * (factor - j) +
			       self->data[(i+1)*factor] * j) / factor;
#ifdef __DEBUG
			printf("[interpolator.c] Interpolating (%f, %f) to (%f %f): (%f %f)\n",
					creal(self->data[i*factor]), cimag(self->data[i*factor]),
					creal(self->data[(i+1)*factor]), cimag(self->data[(i+1)*factor]),
					creal(avg), cimag(avg));
#endif
			self->data[i*factor+j] = avg;
		}
	}

	/* TODO low-pass filtering */

	interpd_offset = count % factor;
	return count;
}

static int
interp_close(Sample *self)
{
	free(self->_backend);
	free(self->data);
	free(self);
	return 0;
}

