/*
 * maxvol.h
 *
 *  Created on: 2022年6月5日
 *      Author: yuexu
 */

#ifndef MAIN_MAXVOL_PWMOUT_H_
#define MAIN_MAXVOL_PWMOUT_H_

#include "common_config.h"
#include "pwmout.h"

#define BIT24_MAX 0x7FFFFF
#define BIT24_MIN 0x800000
#define STEP RATE

static int32_t MAX_NOW = 0;
static int32_t MIN_NOW = 0;
static int _step_num = 0;

int _logi = 0;

static inline int32_t _proc_maxvol(int32_t data) {
	if (_step_num < STEP) {
		if (data > MAX_NOW) MAX_NOW = data;
		if (data < MIN_NOW) MIN_NOW = data;
		_step_num++;
	} else {
		_step_num = 0;
		MAX_NOW = MAX_NOW / 3;
		MIN_NOW = MIN_NOW / 3;
	}

	int32_t ret = 0;
	if (data > 0) ret = (BIT24_MAX / MAX_NOW) * data;
	else ret = (BIT24_MIN / MIN_NOW) * data;

	//if ((_logi++ % 1000) == 0) ESP_LOGI(LOG_TAG_PWMOUT, "ret: %d, data: %d, max_now: %d, min_now: %d, step: %d, 24max: %d", ret, data, MAX_NOW, MIN_NOW, _step_num, BIT24_MAX);

	return ret;
}



/**
 * 注意：这只是为了得到一个和音量无关的最大化电平控制的信号，通过pwm输出。
 * 调整频率大概： RATE * 2 / BUF_SIZE (48000 * 2 / 256 = 375)
 */
static inline void maxvol(int32_t* src, int32_t* dst, int len, int ch) {
	int i;
	int32_t data = 0;
	int32_t total = 0;
	int32_t max = 0;

	for (i = 0; i < len; i += 2) {
		int32_t rl = src[i];
		int32_t rr = src[i+1];

		rl = rl >> 8;
		rr = rr >> 8;

		if (ch == L_CODE) {
			data = _proc_maxvol(rl);
		} else if (ch == R_CODE) {
			data = _proc_maxvol(rr);
		} else {
			data = _proc_maxvol((rr + rl) / 2);
		}
		if (data < 0) data = -data;
		if (data > max) max = data;
		total += data;
	}

	int32_t val = max * 2 / 3 + (total * 2 / i) / 3;
	float duty = (float)val / BIT24_MAX;
	if (duty > 1) duty = 0.9;
	pwm_set_duty(duty);

	if ((_logi++ % 1000) == 0) ESP_LOGI(LOG_TAG_PWMOUT, "duty: %f, total: %d, max: %d, val: %d, 24max: %d", duty, total, max, val, BIT24_MAX);
}

#endif /* MAIN_MAXVOL_PWMOUT_H_ */
