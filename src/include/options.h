/**
 * All accepted short and long command-line flags are defined here
 */
#ifndef _METEOR_OPTIONS_H
#define _METEOR_OPTIONS_H

#include <getopt.h>
#include <stdlib.h>

#define SHORTOPTS "o:b:r:R:wo:hv"

struct option longopts[] = {
	{ "output",       1, NULL, 'o' },
	{ "pll-bw",       1, NULL, 'b' },
	{ "rate",         1, NULL, 'r' },
	{ "wait",         0, NULL, 'w' },
	{ "refresh-rate", 1, NULL, 'R' },
	{ "oversamp",     1, NULL, 's' },
	{ "help",         0, NULL, 'h' },
	{ "version",      0, NULL, 'v' },
};


#endif
