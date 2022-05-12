#include <stdio.h>
#include <stdlib.h>
#include "snd.h"

int main(int argc, char* argv[]) {
	int rc;
	snd_pcm_t* handle;

	char* buffer;
	snd_pcm_uframes_t frames;
	
	int size = 0;
	
	char* hw;
	if (argc < 2) {
		fprintf(stderr, "usage play hw:0\n");
		exit(1);
	}
	hw = argv[1];

	frames = c_snd_pcm(&handle, hw, 0);

	size = frames * FRAME_BUFFER_X;
	buffer = (char*) malloc(size);

	fprintf(stderr, "play buffer size: %d frames: %ld\n", size, frames);

	while(1) {
		//memset(buffer, 0, size);
		rc = fread(buffer, 1, size, stdin);
		if (rc < 0) {
			fprintf(stderr, "play error read from stdin: %s\n",
				  strerror(rc));
			break;
		} else if (rc != size) {
			fprintf(stderr, "play short read, read %d len\n", rc);
			break;
		}

		while((rc = snd_pcm_writei(handle, buffer, frames)) < 0) {
			if (rc == -EPIPE) {
				fprintf(stderr, "play underrun occurred\n");
				snd_pcm_prepare(handle);
			} else if (rc < 0) {
				fprintf(stderr,
					"error from writei: %s\n",
					snd_strerror(rc));
			}
		}
   }
   snd_pcm_drain(handle);
   snd_pcm_close(handle);
   free(buffer);
}
