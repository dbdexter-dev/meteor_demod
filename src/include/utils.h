/**
 * Various utility functions that don't really fit in any other file.
 */
#ifndef _METEOR_UTILS_H
#define _METEOR_UTILS_H

#include <complex.h>
#include <stdlib.h>

char  clamp(float x);
float float_clamp(float x, float max_abs);
int   slice(float x);

void  humanize(size_t count, char *buf);
char* gen_fname();

void  usage(char *pname);
void  fatal(char *msg);
void  splash();
void  version();
void* safealloc(size_t size);

#endif
