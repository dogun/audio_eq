#include <stdio.h>
#include <stdlib.h>
#include "snd.h"

#define BIT24_MAX 0x7FFFFF
#define BIT24_MIN 0x800000
#define STEP RATE

static float MAX_NOW = 1;
static float MIN_NOW = -1;
static int _step_num = 0;

static inline int32_t _proc_maxvol(int32_t data) {
	if (_step_num < STEP) {
		if (data > MAX_NOW) MAX_NOW = data;
		if (data < MIN_NOW) MIN_NOW = data;
		_step_num++;
	} else {
		_step_num = 0;
		MAX_NOW = MAX_NOW / 3;
		MIN_NOW = MIN_NOW / 3;
		if (MAX_NOW == 0) MAX_NOW = 1;
		if (MIN_NOW == 0) MIN_NOW = -1;
	}

	int32_t ret = 0;
	if (data > 0) ret = ((float)BIT24_MAX / MAX_NOW) * data;
	else ret = ((float)BIT24_MIN / MIN_NOW) * data;

	printf("o: %d, r: %d, max: %f, min: %f\n", data, ret, MAX_NOW, MIN_NOW);

	return ret;
}


int main(int argc, char* argv[]) {
	char* buffer;
	int size = 0;
	int rc;
	
	char* channel;
	if (argc < 2) {
		fprintf(stderr, "usage maxvol l/r \n");
		exit(1);
	}
	channel = argv[1];
	
	size = FRAME_SIZE * FRAME_BUFFER_X;
	buffer = (char*)malloc(size);

	fprintf(stderr, "maxvol buffer size: %d frames: %d\n", size, FRAME_SIZE);
	
	int i, data_l, data_r;
	int t_data = 0;
	int* t_buffer;

	while(1) {
		rc = fread(buffer, 1, size, stdin);
		if (rc < 0) {
			fprintf(stderr, "maxvol error read from stdin: %s\n",
				  strerror(rc));
			break;
		} else if (rc != size) {
			fprintf(stderr, "maxvol short read, read %d len\n", rc);
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
			t_data = t_data >> 8;
			
			int max_data = _proc_maxvol(t_data);
		}
   }
   free(buffer);
}

