#ifndef wavfile_h
#define wavfile_h

#include <complex.h>
#include <stdio.h>

/**
 * Parse the WAV header in a file if available, and seek the file descriptor to
 * the start of the raw samples array
 *
 * @param fd wav file descriptor
 * @param samplerate pointer filled with the samplerate
 * @param bps pointer filled with the number of bits per sample
 * @param samples_start offset of the samples array from the start of the file
 *
 * @return 0 on success
 *         1 if the file is not a valid WAV file
 */
int wav_parse(FILE *fd, int *samplerate, int *bps);

/**
 * Read a sample from the given wav file, converting it to fpcomplex_t and
 * normalizing
 *
 * @param dst pointer to the destination sample
 * @param bps bits per sample of the wav file
 * @param fd descriptor of the wav file, pointing to the next sample to read
 * @return 0 on success
 *         1 on failure
 */
int wav_read(float complex *dst, int bps, FILE *fd);

#endif
