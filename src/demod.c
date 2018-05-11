#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "demod.h"
#include "utils.h"

#define SAMPLE_MAX 256

static double get_phase(uint8_t i, uint8_t q);
static double get_mag(uint8_t i, uint8_t q);

static double* phase_table;
static double* mag_table;

void
demod_init()
{
	int i, q;

	phase_table = safealloc(sizeof(*phase_table) * SAMPLE_MAX * SAMPLE_MAX);
	mag_table = safealloc(sizeof(*mag_table) * SAMPLE_MAX * SAMPLE_MAX);

	/* Populate the two lookup tables */
	for (i=0; i<SAMPLE_MAX; i++) {
		for (q=0; q<SAMPLE_MAX; q++) {
			phase_table[SAMPLE_MAX*i+q] = get_phase(i, q);
			mag_table[SAMPLE_MAX*i+q] = get_mag(i, q);
		}
	}
}

void
demod_deinit()
{
	free(phase_table);
	free(mag_table);
}


double
get_phase(uint8_t i, uint8_t q)
{
	return atan((double)i/q);
}

double
get_mag(uint8_t i, uint8_t q)
{
	return sqrt(i*i+q*q);
}
