#include <stdio.h>
#include "wavfile.h"
#include "utils.h"

#define CHUNKSIZE 512

int
main(int argc, char *argv[])
{
	int i;
	int err;
	Sample samp;

	if (argc < 2) {
		usage(argv[0]);
	}

	err = open_samples_file(&samp, argv[1]);

	if (err) {
		printf("[ERROR] %d\n", err);
		return err;
	}

	samp.read(&samp, CHUNKSIZE);

	for (i=0; i<CHUNKSIZE; i++) {
		printf("%d ", samp.data[i]);
	}
	
	return 0;
}
