#include <stdlib.h>
#include <stdio.h>

void
usage(char *pname)
{
	fprintf(stderr, "Usage: %s <file to decode>\n", pname);
	exit(0);
}

void*
safealloc(size_t size)
{
	void* ptr;
	ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "Failed to allocate block of size %lu", size);
		exit(-1);
	}

	return ptr;
}
