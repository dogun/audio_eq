/*
 * val_map.h
 *
 *  Created on: 2022Äê7ÔÂ13ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_VAL_MAP_H_
#define MAIN_VAL_MAP_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "common_config.h"

typedef struct {
	float input_start;
	float input_end;
	float output_start;
	float output_end;
} Val_map;

Val_map* create_val_map(float input_start, float input_end, float output_start, float output_end) {
	Val_map* val_map = (Val_map*)malloc(sizeof(Val_map));
	val_map->input_start = input_start;
	val_map->input_end = input_end;
	val_map->output_start = output_start;
	val_map->output_end = output_end;
	return val_map;
}

void free_val_map(Val_map* val_map) {
	if (NULL == val_map) return;
	free(val_map);
}

float vol_map(Val_map* val_map, float input) {
	if (input > val_map->input_start) {
		input = val_map->output_start
				+ (input - val_map->input_start)
						* ((val_map->output_end - val_map->output_start)
								/ (val_map->input_end - val_map->input_start));
	}
	return input;
}

#endif /* MAIN_VAL_MAP_H_ */
