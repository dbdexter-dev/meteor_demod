/**
 * All accepted short and long command-line flags are defined here
 */
#ifndef _METEOR_OPTIONS_H
#define _METEOR_OPTIONS_H

#include <getopt.h>
#include <stdlib.h>

#define SHORTOPTS "b:Bho:r:R:O:vw"

struct option longopts[] = {
	{ "pll-bw",       1, NULL, 'b' },
	{ "batch",        1, NULL, 'B' },
	{ "help",         0, NULL, 'h' },
	{ "output",       1, NULL, 'o' },
	{ "oversamp",     1, NULL, 'O' },
	{ "rate",         1, NULL, 'r' },
	{ "refresh-rate", 1, NULL, 'R' },
	{ "version",      0, NULL, 'v' },
	{ "wait",         0, NULL, 'w' },
};


#endif
