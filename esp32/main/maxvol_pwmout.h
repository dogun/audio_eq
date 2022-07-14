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
#define STEP RATE * 2  //最大音量的统计周期，乘数为秒数

const static float MAX_NOW_LIMIT = BIT24_MAX / 400;

static float MAX_NOW = MAX_NOW_LIMIT;

static int _step_num = 0;

int _logi = 0;

static inline int32_t _proc_maxvol(int32_t data) {
	int32_t r_data = abs(data);
	if (_step_num < STEP) {
		if (r_data > MAX_NOW) MAX_NOW = r_data;
		_step_num++;
	} else {
		_step_num = 0;
		MAX_NOW = MAX_NOW_LIMIT;
	}

	int32_t ret = ((float)BIT24_MAX / MAX_NOW) * data;

	//float pp = (float)ret / data;

	//if ((_logi++ % 10000) == 0) ESP_LOGI(LOG_TAG_PWMOUT, "%2.2f, ret: %d, data: %d, max_now: %d, step: %d, 24max: %d", pp, ret, data, (int)MAX_NOW, _step_num, BIT24_MAX);

	//if (r_data < BIT24_MAX / 10000) ret = 0;

	return ret;
}

/**
 * 压缩： [start1, end1] -> [start2, end2]
 */
static inline float _n(float duty, float start1, float end1, float start2, float end2) {
	if (duty > start1) {
		duty = start2 + (duty - start1) * ((end2 - start2) / (end1 - start1));
	}
	return duty;
}

/**
 * 注意：这只是为了得到一个和音量无关的最大化电平控制的信号，通过pwm输出。
 * 调整频率大概： RATE * 2 / BUF_SIZE (48000 * 2 / 256 = 375)
 */
static inline void maxvol(int32_t* src, int32_t* dst, int len, int ch, float up_start) {
	int i;
	float data = 0;
	float total = 0;
	float max = 0;

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

	float val = max * 2 / 3 + (total * 2 / i) / 3;
	float duty = val / BIT24_MAX;

	duty = _n(duty, 0.1, 1, up_start, 1);

	if (duty > 1) duty = 1;

	pwm_set_duty(duty);

	//if ((_logi++ % 1000) == 0) ESP_LOGI(LOG_TAG_PWMOUT, "duty: %f, total: %d, max: %d, val: %d, 24max: %d", duty, total, max, val, BIT24_MAX);
}

#endif /* MAIN_MAXVOL_PWMOUT_H_ */
