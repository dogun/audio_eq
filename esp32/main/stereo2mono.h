/*
 * stereo2mono.h
 *
 *  Created on: Jun 2, 2022
 *      Author: yuexu
 */

#ifndef MAIN_STEREO2MONO_H_
#define MAIN_STEREO2MONO_H_

static inline void stereo2mono(int32_t* src, int32_t* dst, int len) {
	int i;

	for (i = 0; i < len; i += 2) {
		int32_t rl = src[i];
		int32_t rr = src[i+1];

		rl = rl >> 8;
		rr = rr >> 8;

		rl = (rl + rr) / 2;

		rl = rl << 8;

		dst[i] = rl;
		dst[i+1] = rl;
	}
}

#endif /* MAIN_STEREO2MONO_H_ */

