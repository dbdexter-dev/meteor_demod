#ifndef utils_h
#define utils_h

#include <stdlib.h>

#ifndef VERSION
#define VERSION "(unknown version)"
#endif


#define DO_PRAGMA(x) _Pragma(#x)

/* Portable unroll pragma, for some reason clang defines __GNUC__ but uses the
 * non-GCC unroll pragma format */
#if defined(__clang__)
#define PRAGMA_UNROLL(x) DO_PRAGMA(unroll x)
#elif defined(__GNUC__)
#define PRAGMA_UNROLL(x) DO_PRAGMA(GCC unroll x)
#else
#define PRAGMA_UNROLL(x) DO_PRAGMA(unroll x)
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define LEN(x) (sizeof(x)/sizeof(x[0]))
#ifndef sgn
#define sgn(x) ((x) < 0 ? -1 : 1)
#endif

/**
 * Generate a symbol filename based on the current date and time
 *
 * @return filename
 */
char* gen_fname();

/**
 * Format a number into a more readable format
 *
 * @param value value to
 * @param buf buffer to write the resulting string to
 */
void humanize(size_t value, char *buf);

/**
 * Format a number of seconds into HH:MM:SS format
 *
 * @param secs seconds
 * @param buf buffer to write the resulting string to
 */
void seconds_to_str(unsigned secs, char *buf);

/**
 * Convert a "human-readable" number into a float
 * e.g. 137.1M = 137100.0
 *
 * @param human string to parse
 * @return parsed float
 */
float human_to_float(const char *human);

/**
 * Write usage info to stdout
 *
 * @param progname executable name
 */
void  usage(const char *progname);

/**
 * Write version info to stdout
 */
void  version();

#endif
