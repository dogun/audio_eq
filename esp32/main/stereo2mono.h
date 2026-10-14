/*
 * stereo2mono.h
 *
 *  Created on: Jun 2, 2022
 *      Author: yuexu
 */

#ifndef MAIN_STEREO2MONO_H_
#define MAIN_STEREO2MONO_H_

#include "audio_sample.h"

static inline void stereo2mono(int32_t* src, int32_t* dst, int len) {
	int i;

	for (i = 0; i < len; i += 2) {
		int32_t rl = audio_i2s_to_s24(src[i]);
		int32_t rr = audio_i2s_to_s24(src[i+1]);

		rl = (rl + rr) / 2;
		rl = audio_s24_to_i2s(rl);

		dst[i] = rl;
		dst[i+1] = rl;
	}
}

#endif /* MAIN_STEREO2MONO_H_ */

