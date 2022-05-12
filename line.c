#include <stdio.h>
#include <stdlib.h>
#include "snd.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

int main(int argc, char* argv[]) {
	char* buffer;
	int size = 0;
	int rc;
	
	char* channel;
	int sample = 100;
	if (argc < 3) {
		fprintf(stderr, "usage line l/r sample\n");
		exit(1);
	}
	channel = argv[1];
	sample = atoi(argv[2]);
	
	size = FRAME_SIZE * FRAME_BUFFER_X;
	buffer = (char*)malloc(size);

	fprintf(stderr, "line buffer size: %d frames: %d\n", size, FRAME_SIZE);
	
	int i, data_l, data_r;
	int j = 0;
	int data_1 = 0;
	int data_2 = 0;
	int t_data = 0;
	int* t_buffer;

	while(1) {
		rc = fread(buffer, 1, size, stdin);
		if (rc < 0) {
			fprintf(stderr, "line error read from stdin: %s\n",
				  strerror(rc));
			break;
		} else if (rc != size) {
			fprintf(stderr, "line short read, read %d len\n", rc);
			break;
		}

		t_buffer = (int*)buffer;
		for (i = 0; i < size / 4; i = i + 2) {
			data_l = *(t_buffer + i);
			data_r = *(t_buffer + i + 1);
			if (strcmp("l", channel) == 0)
				t_data = data_l;
			else
				t_data = data_r;
			
			//fprintf(stdout, "==%d %d\n", data_l, data_r);
			if (j >= sample) {
				fprintf(stdout, "%d\t%d\n", data_1, data_2);
				j = 0;
				data_1 = t_data;
				data_2 = t_data;
			}else {
				j++;
				if (t_data > 0 && t_data > data_1) data_1 = t_data;
				if (t_data < 0 && t_data < data_2) data_2 = t_data;
			}
		}
   }
   free(buffer);
}

