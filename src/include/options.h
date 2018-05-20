/**
 * All accepted short and long command-line flags are defined here
 */
#ifndef _METEOR_OPTIONS_H
#define _METEOR_OPTIONS_H

#include <getopt.h>
#include <stdlib.h>

#define SHORTOPTS "b:hvr:s:o:"

struct option longopts[] = {
	{ "bandwidth",  1,  NULL, 'b' },
	{ "output",     1,  NULL, 'o' },
	{ "rate",       1,  NULL, 'r' },
	{ "oversamp",   1,  NULL, 's' },
	{ "help",       0,  NULL, 'h' },
	{ "version",    0,  NULL, 'v' },
};


#endif
