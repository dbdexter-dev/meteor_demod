#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dsp/agc.h"
#include "wavfile.h"

#define FILE_BUFFER_SIZE 32768
static union {
	uint8_t bytes[FILE_BUFFER_SIZE];
	int16_t words[FILE_BUFFER_SIZE/2];
	float floats[FILE_BUFFER_SIZE/4];
} _buffer;
static size_t _offset;

struct wave_header {
	char _riff[4];          /* Literally RIFF */
	uint32_t chunk_size;
	char _filetype[4];      /* Literally WAVE */

	char _fmt[4];           /* Literally fmt  */
	uint32_t subchunk_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char _data[4];          /* Literally data */
	uint32_t subchunk2_size;
};

int
wav_parse(FILE *fd, int *samplerate, int *bps)
{
	struct wave_header header;

	if (!(fread(&header, sizeof(struct wave_header), 1, fd))) return 1;

	if (strncmp(header._riff, "RIFF", 4)) return 1;
	if (strncmp(header._filetype, "WAVE", 4)) return 1;
	if (header.num_channels != 2) return 1;

	if (!(*bps = header.bits_per_sample)) return 1;
	*samplerate = (int)header.sample_rate;

	return 0;
}

int
wav_read(float complex *dst, int bps, FILE *fd)
{
	float complex tmp;

	if (!_offset && !fread(&_buffer.bytes, sizeof(_buffer.bytes), 1, fd)) return 0;

	switch (bps) {
		case 8:
			/* Unsigned byte */
			tmp = (int)_buffer.bytes[_offset]-128 + I*((int)_buffer.bytes[_offset+1]-128);
			break;
		case 16:
			/* Signed short */
			tmp = _buffer.words[_offset] + I*_buffer.words[_offset+1];
			break;
		case 32:
			/* Float */
			tmp = _buffer.floats[_offset] + I*_buffer.floats[_offset+1];
			break;
		default:
			return 0;
			break;
	}

	_offset += 2;
	if (_offset*bps/8 >= sizeof(_buffer)) _offset = 0;

	*dst = tmp;
	return 1;
}
