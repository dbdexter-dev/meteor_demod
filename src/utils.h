#ifndef _METEOR_UTILS_H
#define _METEOR_UTILS_H

#include <complex.h>
#include <stdlib.h>

char          clamp(float x);
int           slice(float x);
void          usage(char *pname);
void          fatal(char *msg);
void          version();
void*         safealloc(size_t size);
float         interpolate(float a, float b, float perc);
float complex rotate(float complex i, float arg);

#endif
