#include <stdlib.h>

typedef struct sample
{
	size_t count;
	unsigned bytes_per_sample;
	unsigned samplerate;
	char *data;
	int (*read)(struct sample *, size_t);
} Sample;
