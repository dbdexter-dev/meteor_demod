#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "wavfile.h"

typedef struct {
	FILE *fd;
	size_t total_bytes;
	size_t bytes_read;
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

		fread(&_header, sizeof(struct wave_header), 1, state->fd);

		/* If any of these comparisons return non-zero, the file is
		 * not a valid WAVE file: abort */
		if (!strncmp(_header._riff, "RIFF", 4) &&
			!strncmp(_header._filetype, "WAVE", 4) &&
			!strncmp(_header._data, "data", 4)) {
			samp->count = 0;
			samp->samplerate = _header.sample_rate;
			samp->data = NULL;
			samp->bps = _header.bits_per_sample;
			samp->read = wav_read;
			samp->close = wav_close;
			state->total_bytes = _header.subchunk2_size;
			state->bytes_read = 0;
		}
	} else {
		fatal("Invalid .wav file specified\n");
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

	tmp = malloc(2*self->bps/8);
	for (i=0; i<count; i++) {
		if (fread(tmp, self->bps/8, 2, state->fd) > 0) {
			self->data[i] = tmp[0] + tmp[1] * I;
		} else {
			break;
		}
	}
	free(tmp);

	/* Update the byte count */
	state->bytes_read += i*self->bps/8*2;

	return i;
}

float
wav_get_perc(Sample *self)
{
	const WavState* state = self->_backend;
	return (float)state->bytes_read/state->total_bytes*100;
}

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
