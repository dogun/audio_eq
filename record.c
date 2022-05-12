#include <stdio.h>
#include <stdlib.h>
#include "snd.h"

int main(int argc, char* argv[]) {
	int rc;
	snd_pcm_t* handle;

	char* buffer;
	snd_pcm_uframes_t frames;
	
	int size = 0;
	int time = 0;

	char* hw;
	if (argc < 3) {
		fprintf(stderr, "usage record hw:0 time\n");
		exit(1);
	}
	hw = argv[1];
	time = atoi(argv[2]);
	
	frames = c_snd_pcm(&handle, hw, 1);

	size = frames * FRAME_BUFFER_X;
	buffer = (char*)malloc(size);
	
	float frame_cnt = time * RATE / frames;

	fprintf(stderr, "record buffer size: %d frames: %ld\n", size, frames);

	int i = 0;
	while(1) {
		if (frame_cnt > 0 && i++ > frame_cnt) break;
		
		//memset(buffer, 0, size);
		rc = snd_pcm_readi(handle, buffer, frames);
		if (rc == -EPIPE) {
			fprintf(stderr,"record overrun occured\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr,"error from read: %s\n",
				  snd_strerror(rc));
			break;
		} else if (rc != frames) {
			fprintf(stderr, "record short read, read %d frames\n",rc);
			break;
		}

		fwrite(buffer, 1, size, stdout);
		//fflush(stdout);
   }
   snd_pcm_drain(handle);
   snd_pcm_close(handle);
   free(buffer);
}

