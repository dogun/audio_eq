/*
 * maxvol.h
 *
 *  Created on: 2022年6月5日
 *      Author: yuexu
 */

#ifndef MAIN_MAXVOL_H_
#define MAIN_MAXVOL_H_

#include "common_config.h"

#define BIT24_MAX 0x7FFFFF
#define BIT24_MIN 0x800000
#define STEP RATE

static int32_t MAX_NOW = 0;
static int32_t MIN_NOW = 0;
static int _step_num = 0;

static inline int32_t _proc_maxvol(int32_t data) {
	if (_step_num < STEP) {
		if (data > MAX_NOW) MAX_NOW = data;
		if (data < MIN_NOW) MIN_NOW = data;
	} else {
		_step_num = 0;
		MAX_NOW = MAX_NOW / 2;
		MIN_NOW = MIN_NOW / 2;
	}
	if (data > 0) return data * BIT24_MAX / MAX_NOW;
	else return data * BIT24_MIN / MIN_NOW;
}

/**
 * 注意：会失真，这只是为了得到一个和音量无关的最大化电平控制的信号。
 */
static inline void maxvol(int32_t* src, int32_t* dst, int len, int ch) {
	int i;

	for (i = 0; i < len; i += 2) {
		int32_t rl = src[i];
		int32_t rr = src[i+1];

		rl = rl >> 8;
		rr = rr >> 8;

		if (ch == L_CODE) {
			rl = _proc_maxvol(rl);
		} else if (ch == R_CODE) {
			rr = _proc_maxvol(rr);
		} else {
			rl = _proc_maxvol(rl);
			rr = _proc_maxvol(rr);
		}

		rl = rl << 8;
		rr = rr << 8;

		dst[i] = rl;
		dst[i+1] = rr;
	}
}


#endif /* MAIN_MAXVOL_H_ */
