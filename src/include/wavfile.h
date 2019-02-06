/**
 * Basic functions and structs to deal with I/Q samples coming from a .wav file,
 * as well as raw I/Q recordings like those coming from rtl_fm.
 */
#ifndef METEOR_WAVFILE_H
#define METEOR_WAVFILE_H

#include <stdint.h>
#include "source.h"

struct wave_header
{
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

Source* open_samples_file(const char *fname, unsigned samplerate, unsigned bps);

#endif
