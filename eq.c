#include <stdio.h>
#include <stdlib.h>
#include "eq.h"

int main(int argc, char* argv[]) {
	int rc;

	char* buffer;

	int size = 0;
	
	char* eq_file;
	double vol;
	if (argc < 3) {
		fprintf(stderr, "usage eq eq_file vol\n");
		exit(1);
	}

	eq_file = argv[1];
	char* s_vol = argv[2];
	vol = atof(s_vol) / 100;

	size = FRAME_SIZE * FRAME_BUFFER_X;
	buffer = (char *) malloc(size);

	fprintf(stderr, "eq buffer size: %d frames: %d\n", size, FRAME_SIZE);

	load_eq_config(eq_file);

	while(1) {
		//memset(buffer, 0, size);
		rc = fread(buffer, 1, size, stdin);
		//fprintf(stderr, "eq read from stdin size: %d\n", rc);
		if (rc < 0) {
			fprintf(stderr, "eq error read from stdin: %s\n",
				  strerror(rc));
			break;
		} else if (rc != size) {
			fprintf(stderr, "eq short read, read %d len\n", rc);
			break;
		}
		process_eq(buffer, size, vol);
		rc = fwrite(buffer, 1, size, stdout);
		//fprintf(stderr, "eq write to stdout size: %d\n", rc);
   }

   free(buffer);
}

