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
} WavState;

static int wav_read(Sample *samp, size_t count);
static int wav_close(Sample *samp);

extern int errno;

Sample*
open_samples_file(char *fname)
{
	Sample *samp;
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

		/* If any of these comparisons return non-zero, the file is
		 * not a valid WAVE file: abort */
		if (!strncmp(_header._riff, "RIFF", 4) &&
			!strncmp(_header._filetype, "WAVE", 4) &&
			!strncmp(_header._data, "data", 4)) {
			samp->count = 0;
			samp->samplerate = _header.sample_rate;
			samp->data = NULL;
			samp->bps = _header.bits_per_sample/8;
			samp->read = wav_read;
			samp->close = wav_close;
			state->total_samples = _header.subchunk2_size / _header.num_channels / samp->bps;
			state->samples_read = 0;
		}
	} else {
		fatal("Invalid .wav file specified");
		/* Not reached */
		return NULL;
    }

	return samp;
}

/* Read $count samples from the opened file, populating the data[] array as
 * expected. */
int
wav_read(Sample *self, size_t count)
{
	WavState *state;
	short* tmp;
	int i;

	state = (WavState*)self->_backend;

	if (!self->data) {
		self->data = safealloc(count * sizeof(*self->data));
	} else if (self->count < count) {
		free(self->data);
		self->data = safealloc(count * sizeof(*self->data));
	}

	self->count = count;

	/* Convert samples (aka uint16_t) to complex numbers */
	tmp = malloc(2*self->bps);
	for (i=0; i<count; i++) {
		if (fread(tmp, self->bps, 2, state->fd) > 0) {
			self->data[i] = tmp[0] + tmp[1] * I;
		} else {
			break;
		}
	}
	free(tmp);

	/* Update the byte count */
	state->samples_read += i;

	return i;
}

/* Return how for into the file we are */
float
wav_get_perc(Sample *self)
{
	const WavState* state = self->_backend;
	return (float)state->samples_read/state->total_samples*100;
}

unsigned
wav_get_size(Sample *self)
{
	const WavState* state = self->_backend;
	return state->total_samples;
}

/* Close the .wav file descriptor and free the memory associated with this
 * Sample object */
int
wav_close(Sample *self)
{
	WavState *state;

	state = (WavState*)self->_backend;
	fclose(state->fd);

	free(self->_backend);
	free(self->data);
	free(self);
	return 0;
}
