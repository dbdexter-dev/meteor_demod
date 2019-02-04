#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "wavfile.h"

typedef struct {
	FILE *fd;
	uint64_t total_samples;
	uint64_t samples_read;
} WavState;

static int      wav_read(Source *samp, size_t count);
static int      wav_close(Source *samp);
static uint64_t wav_get_size(const Source *samp);
static uint64_t wav_get_done(const Source *samp);

extern int errno;

Source*
open_samples_file(const char *fname, unsigned samplerate, unsigned bps)
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
			samp->bps = (bps ? bps : _header.bits_per_sample);

			state->total_samples = _header.subchunk2_size / _header.num_channels / (samp->bps / 8);
		} else {
			if (!bps) {
				bps = 16;
			}
			fprintf(stderr, "Warning: input file is not a valid .wav, assuming raw %d bit data\n", bps);
			if (!samplerate) {
				fatal("Please specify an input samplerate (-s <samplerate>)");
				/* Not reached */
				return NULL;
			}

			samp->samplerate = samplerate;
			samp->bps = bps;
			state->total_samples = 0;
		}
		state->samples_read = 0;
	} else {
		fatal("Could not find specified file");
		/* Not reached */
		return NULL;
    }

	return samp;
}

/* Return how for into the file we are */
uint64_t
wav_get_done(const Source *self)
{
	const WavState* state = self->_backend;
	return state->samples_read;
}

/* Return how big the file is */
uint64_t
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
	unsigned i;
	uint8_t tmp_8bit[2];
	int16_t tmp_16bit[2];

	state = (WavState*)self->_backend;

	if (!self->data) {
		self->data = safealloc(count * sizeof(*self->data));
	} else if (self->count < count) {
		free(self->data);
		self->data = safealloc(count * sizeof(*self->data));
	}

	self->count = count;

	/* Convert samples (uint8_t or int16_t) to complex numbers */
	switch(self->bps) {
	case 8:         /* unsigned 8 bits per sample */
		for (i=0; i<count; i++) {
			if (fread(tmp_8bit, 1, 2, state->fd) > 0) {
				self->data[i] = tmp_8bit[0] + tmp_8bit[1] * I;
			} else {
				break;
			}
		}
		break;
	case 16:        /* signed 16 bits per sample */
		for (i=0; i<count; i++) {
			if (fread(tmp_16bit, 2, 2, state->fd) > 0) {
				self->data[i] = tmp_16bit[0] + tmp_16bit[1] * I;
			} else {
				break;
			}
		}
		break;
	default:
		fatal("Unsupported number of bits per sample");
		break;
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

	free(self->_backend);
	free(self->data);
	free(self);
	return 0;
}
/*}}}*/
