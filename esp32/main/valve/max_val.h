/*
 * max_val.h
 *
 *  Created on: 2022Äê7ÔÂ13ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_MAX_VAL_H_
#define MAIN_MAX_VAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct {
	float max;
	float max_now;
	float max_now_limit;
	int multiple;
	int rate;
	int second;
	int max_step;
	int step;
} Max_val;

Max_val* create_max_val(int max, int multiple, int rate, int second) {
	Max_val* max_val = (Max_val*)malloc(sizeof(Max_val));
	max_val->max = max;
	max_val->multiple = multiple;
	max_val->rate = rate;
	max_val->second = second;
	max_val->max_step = max_val->rate * max_val->second;
	max_val->step = 0;
	max_val->max_now = 0;
	max_val->max_now_limit = max_val->max / max_val->multiple;
	return max_val;
}

void free_max_val(Max_val* max_val) {
	if (NULL == max_val) return;
	free(max_val);
}

float max_val(Max_val* max_val, float input) {
	float data = input;
	if (data < 0) data = -data;

	if (max_val->step < max_val->max_step) {
		if (data > max_val->max_now) max_val->max_now = data;
		max_val->step++;
	} else {
		max_val->step = 0;
		max_val->max_now = max_val->max_now_limit;
	}

	float ret = (max_val->max / max_val->max_now) * input;

	return ret;
}

#endif /* MAIN_MAX_VAL_H_ */
