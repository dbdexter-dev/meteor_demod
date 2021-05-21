#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dsp/agc.h"
#include "wavfile.h"

#define FILE_BUFFER_SIZE 32768
static char _buffer[FILE_BUFFER_SIZE];

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

	setvbuf(fd, _buffer, _IOFBF, FILE_BUFFER_SIZE);

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
	float iq32[2];
	uint8_t iq8[2];
	int16_t iq16[2];

	switch (bps) {
		case 8:
			/* Unsigned byte */
			if (!fread(&iq8, sizeof(iq8), 1, fd)) return 0;
			tmp = iq8[0]-128 + I * (iq8[1]-128);
			break;
		case 16:
			/* Signed short */
			if (!fread(&iq16, sizeof(iq16), 1, fd)) return 0;
			tmp = iq16[0] + I * iq16[1];
			break;
		case 32:
			/* Float */
			if (!fread(&iq32, sizeof(iq32), 1, fd)) return 0;
			tmp = iq32[0] + I * iq32[1];
			break;
		default:
			return 0;
			break;
	}

	*dst = tmp;
	return 1;
}
