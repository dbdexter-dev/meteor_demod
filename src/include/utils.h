/**
 * Various utility functions that don't really fit in any other file.
 */
#ifndef METEOR_UTILS_H
#define METEOR_UTILS_H

#define MAX(X, Y) ((X) > (Y)) ? X : Y
#define MIN(X, Y) ((X) < (Y)) ? X : Y

#include <complex.h>
#include <stdlib.h>

typedef enum {
	QPSK,
	OQPSK,
} ModScheme;

int8_t clamp(float x);
float  float_clamp(float x, float max_abs);
int    slice(float x);

void   humanize(size_t count, char *buf);
int    dehumanize(const char *buf);
char*  gen_fname(void);
void   seconds_to_str(unsigned secs, char *buf);

ModScheme parse_mode(char *str);

void   usage(const char *pname);
void   fatal(const char *msg);
void   splash(void);
void   version(void);
void*  safealloc(size_t size);

#endif
