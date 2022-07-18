/*
 * buffer.h
 *
 *  Created on: 2022Äê7ÔÂ15ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_OUTPUT_BUFFER_H_
#define MAIN_OUTPUT_BUFFER_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct {
	int32_t* read_buf;
	int32_t* l_buf;
	int32_t* r_buf;
	int32_t* out_buf;
	int buf_len;
} BIG_DATA_BUFFER;

typedef struct {
	int32_t* buf;
	int len;
} DATA_BUFFER;


#endif /* MAIN_OUTPUT_BUFFER_H_ */
