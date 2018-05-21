/**
 * All accepted short and long command-line flags are defined here
 */
#ifndef _METEOR_OPTIONS_H
#define _METEOR_OPTIONS_H

#include <getopt.h>
#include <stdlib.h>

#define SHORTOPTS "b:hnvr:s:o:"

struct option longopts[] = {
	{ "bandwidth",  1,  NULL, 'b' },
	{ "help",       0,  NULL, 'h' },
	{ "net",        0,  NULL, 'n' },
	{ "port",       1,  NULL, 'p' },
	{ "rate",       1,  NULL, 'r' },
	{ "oversamp",   1,  NULL, 's' },
	{ "version",    0,  NULL, 'v' },
};


#endif
