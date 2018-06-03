#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "wavfile.h"

typedef struct {
	FILE *fd;
	size_t total_samples;
	size_t samples_read;
	int16_t *tmp;
} WavState;

static int      wav_read(Source *samp, size_t count);
static int      wav_close(Source *samp);
static unsigned wav_get_size(const Source *samp);
static unsigned wav_get_done(const Source *samp);

extern int errno;

Source*
open_samples_file(const char *fname, unsigned samplerate)
{
	Source *samp;
	WavState *state;
	struct wave_header _header;
	FILE *fd;

	errno = 0;
	if ((fd = fopen(fname, "r"))) {
		samp = safealloc(sizeof(*samp));
		samp->_backend = safealloc(sizeof(WavState));
		state = (WavState*)samp->_backend;
		state->fd = fd;

		assert(fread(&_header, sizeof(struct wave_header), 1, state->fd));

		samp->count = 0;
		samp->data = NULL;
		samp->read = wav_read;
		samp->close = wav_close;
		samp->size = wav_get_size;
		samp->done = wav_get_done;

		/* If any of these comparisons return non-zero, the file is
		 * not a valid WAVE file: assume raw data */
		if (!strncmp(_header._riff, "RIFF", 4) &&
			!strncmp(_header._filetype, "WAVE", 4) &&
			!strncmp(_header._data, "data", 4)) {
			samp->samplerate = (samplerate ? samplerate : _header.sample_rate);
			samp->bps = _header.bits_per_sample/8;

			state->total_samples = _header.subchunk2_size / _header.num_channels / samp->bps;
		} else {
			fprintf(stderr, "Warning: input file is not a valid .wav, assuming raw 16 bit data\n");

			if (!samplerate) {
				fatal("Please specify an input samplerate (-s <samplerate>)");
				/* Not reached */
				return NULL;
			}

			samp->samplerate = samplerate;
			samp->bps = 2;
			state->total_samples = 0;
		}
		state->samples_read = 0;
		state->tmp = safealloc(2*sizeof(state->tmp));
	} else {
		fatal("Could not find specified file");
		/* Not reached */
		return NULL;
    }

	return samp;
}

/* Return how for into the file we are */
unsigned
wav_get_done(const Source *self)
{
	const WavState* state = self->_backend;
	return state->samples_read;
}

unsigned
wav_get_size(const Source *self)
{
	const WavState* state = self->_backend;
	return state->total_samples;
}

/* Static functions {{{ */
/* Read $count samples from the opened file, populating the data[] array as
 * expected. */
int
wav_read(Source *self, size_t count)
{
	WavState *state;
	int i;

	state = (WavState*)self->_backend;

	if (!self->data) {
		self->data = safealloc(count * sizeof(*self->data));
	} else if (self->count < count) {
		free(self->data);
		self->data = safealloc(count * sizeof(*self->data));
	}

	self->count = count;

	/* Convert samples (aka int16_t) to complex numbers */
	for (i=0; i<count; i++) {
		if (fread(state->tmp, self->bps, 2, state->fd) > 0) {
			self->data[i] = state->tmp[0] + state->tmp[1] * I;
		} else {
			break;
		}
	}

	/* Update the byte count */
	state->samples_read += i;

	return i;
}

/* Close the .wav file descriptor and free the memory associated with this
 * Source object */
int
wav_close(Source *self)
{
	WavState *state;

	state = (WavState*)self->_backend;
	fclose(state->fd);
	free(state->tmp);

	free(self->_backend);
	free(self->data);
	free(self);
	return 0;
}
/*}}}*/
