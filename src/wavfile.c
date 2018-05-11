#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "wavfile.h"

static struct wave_header _header;
static FILE* _fd;
extern int errno;

int
open_samples_file(Sample *samp, char *fname)
{
	errno = 0;

	if ((_fd = fopen(fname, "r"))) {
		fread(&_header, sizeof(struct wave_header), 1, _fd);

		/* If any of these comparisons return non-zero, the file is
		 * not a valid WAVE file: abort */
		if (!strncmp(_header._riff, "RIFF", 4) &&
			!strncmp(_header._filetype, "WAVE", 4) &&
			!strncmp(_header._data, "data", 4)) {
			samp->count = 0;
			samp->samplerate = _header.sample_rate;
			samp->data = NULL;
			samp->bytes_per_sample = _header.bits_per_sample / 8;
			samp->read = read_next;
		} else {
			return -1;
		}
	}

	return errno;
}

int
read_next(Sample *samp, size_t count)
{
	if (!samp->data) {
		samp->data = safealloc(2 * count * _header.bits_per_sample / 8);
	} else if (samp->count < count) {
		free(samp->data);
		samp->data = safealloc(2 * count * _header.bits_per_sample / 8);
	}

	samp->count = count;

	return fread(samp->data, _header.bits_per_sample / 8, count, _fd);
}

int
close_samples_file()
{
	fclose(_fd);
	return 0;
}
