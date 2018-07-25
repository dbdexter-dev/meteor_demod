#include <math.h>
#include <stdlib.h>
#include "filters.h"
#include "interpolator.h"
#include "utils.h"

static int      interp_read(Source *self, size_t count);
static int      interp_free(Source *self);
static uint64_t interp_get_done(const Source *self);
static uint64_t interp_get_size(const Source *self);

typedef struct {
	Source *src;
	Filter *rrc;
	unsigned factor;
} InterpState;

/* Initialize the interpolator, which will use a RRC filter at its core */
Source*
interp_init(Source* src, float alpha, unsigned order, unsigned factor, int sym_rate)
{
	Source *interp;
	InterpState *status;

	interp = safealloc(sizeof(*interp));

	interp->count = 0;
	interp->samplerate = src->samplerate * factor;
	interp->data = NULL;
	interp->bps = sizeof(*src->data);
	interp->read = interp_read;
	interp->close = interp_free;
	interp->done = interp_get_done;
	interp->size = interp_get_size;

	interp->_backend = safealloc(sizeof(InterpState));
	status = (InterpState*) interp->_backend;

	status->factor = factor;
	status->src = src;
	status->rrc = filter_rrc(order, factor, src->samplerate/(float)sym_rate, alpha);

	return interp;
}

/* Static functions {{{ */
/* Wrapper to interpolate the source data and provide a transparent translation
 * layer between the raw samples and the interpolated samples */
uint64_t
interp_get_size(const Source *self)
{
	InterpState *state;
	state = (InterpState*)self->_backend;
	return state->src->size(state->src);
}

uint64_t
interp_get_done(const Source *self)
{
	InterpState *state;
	state = (InterpState*)self->_backend;
	return state->src->done(state->src);
}

int
interp_read(Source *const self, size_t count)
{
	InterpState *status;
	Filter *rrc;
	Source *src;
	unsigned i;
	int factor;
	size_t true_samp_count;

	/* Retrieve the backend info */
	status = (InterpState*)self->_backend;
	factor = status->factor;
	src = status->src;
	rrc = status->rrc;

	/* Prepare a buffer for the data that will go into the Source struct */
	if (!self->data) {
		self->data = safealloc(sizeof(*self->data) * count);
	} else if (self->count < count) {
		free(self->data);
		self->data = safealloc(sizeof(*self->data) * count);
	}

	self->count = count;
	true_samp_count = count / factor;

	/* Read the true samples from the associated source */
	true_samp_count = src->read(src, true_samp_count);
	if (!true_samp_count) {
		return 0;
	}

	/* Feed through the filter, with zero-order hold interpolation */
	for (i=0; i<count; i++) {
		self->data[i] = filter_fwd(rrc, src->data[i/factor]);
	}

	return count;
}

/* Free the memory related to the interpolator. Note that this function does not
 * try to close the underlying data source (aka self->_backend->src) */
int
interp_free(Source *self)
{
	filter_free(((InterpState*)(self->_backend))->rrc);
	free(self->_backend);
	free(self->data);
	free(self);
	return 0;
}
/*}}}*/
