#ifndef _METEOR_UTILS_H
#define _METEOR_UTILS_H

#include <complex.h>
#include <stdlib.h>

char          clamp(float x);
float         float_clamp(float x, float max_abs);
int           slice(float x);
void          usage(char *pname);
void          fatal(char *msg);
void          version();
void          humanize(size_t count, char *buf);
void*         safealloc(size_t size);
float         interpolate(float a, float b, float perc);
float complex rotate(float complex i, float arg);

#endif
