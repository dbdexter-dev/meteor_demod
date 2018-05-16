#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "wavfile.h"

static int wav_read(Sample *samp, size_t count);
static int wav_close(Sample *samp);

static FILE* _fd;
extern int errno;

Sample*
open_samples_file(char *fname)
{
	Sample *samp;
	struct wave_header _header;

	errno = 0;
	if ((_fd = fopen(fname, "r"))) {
		samp = safealloc(sizeof(*samp));
		fread(&_header, sizeof(struct wave_header), 1, _fd);

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
#ifdef __DEBUG
			printf("Opened file %s:\n", fname);
			printf("Sample rate:       %d\n", samp->samplerate);
			printf("Bits per sample:   %d\n", samp->bps);
#endif
		} else {
			return NULL;
		}
	}

	return samp;
}

int
wav_read(Sample *self, size_t count)
{
	short* tmp;
	int i;

	if (!self->data) {
		self->data = safealloc(count * sizeof(*self->data));
	} else if (self->count < count) {
		free(self->data);
		self->data = safealloc(count * sizeof(*self->data));
	}

	self->count = count;

	tmp = malloc(2*self->bps/8);
	for (i=0; i<count; i++) {
		if (fread(tmp, self->bps/8, 2, _fd) > 0) {
			self->data[i] = tmp[0] + tmp[1] * I;
		} else {
			break;
		}
	}

	free(tmp);
	return i;
}

int
wav_close(Sample *self)
{
	fclose(_fd);
	free(self);
	return 0;
}
