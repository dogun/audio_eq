/*
 * filter.h
 *
 *  Created on: 2022Äê7ÔÂ11ÈÕ
 *      Author: https://github.com/adis300/filter-c
 */

#ifndef MAIN_FILTER_H_
#define MAIN_FILTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common_config.h"

#ifndef M_PI
#define M_PI 3.14159265359
#endif

typedef struct {
	int n;
	float* A;
	float* d1;
	float* d2;
	float* w0;
	float* w1;
	float* w2;
} BWLowPass;
typedef BWLowPass BWHighPass;

typedef struct {
	int n;
	float* A;
	float* d1;
	float* d2;
	float* d3;
	float* d4;
	float* w0;
	float* w1;
	float* w2;
	float* w3;
	float* w4;
} BWBandPass;

typedef struct {
	int n;
	float r;
	float s;
	float* A;
	float* d1;
	float* d2;
	float* d3;
	float* d4;
	float* w0;
	float* w1;
	float* w2;
	float* w3;
	float* w4;
} BWBandStop;

BWLowPass* create_bw_low_pass_filter(int order, float sampling_frequency,
		float half_power_frequency);
BWHighPass* create_bw_high_pass_filter(int order, float sampling_frequency,
		float half_power_frequency);
BWBandPass* create_bw_band_pass_filter(int order, float sampling_frequency,
		float lower_half_power_frequency, float upper_half_power_frequency);
BWBandStop* create_bw_band_stop_filter(int order, float sampling_frequency,
		float lower_half_power_frequency, float upper_half_power_frequency);

void free_bw_low_pass(BWLowPass* filter);
void free_bw_high_pass(BWHighPass* filter);
void free_bw_band_pass(BWBandPass* filter);
void free_bw_band_stop(BWBandStop* filter);

float bw_low_pass(BWLowPass* filter, float input);
float bw_high_pass(BWHighPass* filter, float input);
float bw_band_pass(BWBandPass* filter, float input);
float bw_band_stop(BWBandStop* filter, float input);

BWLowPass* create_bw_low_pass_filter(int order, float s, float f) {
	BWLowPass* filter = (BWLowPass*) malloc(sizeof(BWLowPass));
	filter->n = order / 2;
	filter->A = (float*) malloc(filter->n * sizeof(float));
	filter->d1 = (float*) malloc(filter->n * sizeof(float));
	filter->d2 = (float*) malloc(filter->n * sizeof(float));
	filter->w0 = (float*) calloc(filter->n, sizeof(float));
	filter->w1 = (float*) calloc(filter->n, sizeof(float));
	filter->w2 = (float*) calloc(filter->n, sizeof(float));

	float a = tanf(M_PI * f / s);
	float a2 = a * a;
	float r;

	int i;

	for (i = 0; i < filter->n; ++i) {
		r = sinf(M_PI * (2.0 * i + 1.0) / (4.0 * filter->n));
		s = a2 + 2.0 * a * r + 1.0;
		filter->A[i] = a2 / s;
		filter->d1[i] = 2.0 * (1 - a2) / s;
		filter->d2[i] = -(a2 - 2.0 * a * r + 1.0) / s;
	}
	return filter;
}
BWHighPass* create_bw_high_pass_filter(int order, float s, float f) {
	BWHighPass* filter = (BWHighPass*) malloc(sizeof(BWHighPass));
	filter->n = order / 2;
	filter->A = (float*) malloc(filter->n * sizeof(float));
	filter->d1 = (float*) malloc(filter->n * sizeof(float));
	filter->d2 = (float*) malloc(filter->n * sizeof(float));
	filter->w0 = (float*) calloc(filter->n, sizeof(float));
	filter->w1 = (float*) calloc(filter->n, sizeof(float));
	filter->w2 = (float*) calloc(filter->n, sizeof(float));

	float a = tanf(M_PI * f / s);
	float a2 = a * a;
	float r;

	int i;

	for (i = 0; i < filter->n; ++i) {
		r = sinf(M_PI * (2.0 * i + 1.0) / (4.0 * filter->n));
		s = a2 + 2.0 * a * r + 1.0;
		filter->A[i] = 1.0 / s;
		filter->d1[i] = 2.0 * (1 - a2) / s;
		filter->d2[i] = -(a2 - 2.0 * a * r + 1.0) / s;
	}
	return filter;
}
BWBandPass* create_bw_band_pass_filter(int order, float s, float fl, float fu) {
	if (fu <= fl) {
		ESP_LOGE(FILTER_TAG,
				"ERROR:Lower half-power frequency is smaller than higher half-power frequency");
		return NULL;
	}
	BWBandPass* filter = (BWBandPass*) malloc(sizeof(BWBandPass));
	filter->n = order / 4;
	filter->A = (float*) malloc(filter->n * sizeof(float));
	filter->d1 = (float*) malloc(filter->n * sizeof(float));
	filter->d2 = (float*) malloc(filter->n * sizeof(float));
	filter->d3 = (float*) malloc(filter->n * sizeof(float));
	filter->d4 = (float*) malloc(filter->n * sizeof(float));

	filter->w0 = (float*) calloc(filter->n, sizeof(float));
	filter->w1 = (float*) calloc(filter->n, sizeof(float));
	filter->w2 = (float*) calloc(filter->n, sizeof(float));
	filter->w3 = (float*) calloc(filter->n, sizeof(float));
	filter->w4 = (float*) calloc(filter->n, sizeof(float));

	float a = cosf(M_PI * (fu + fl) / s) / cosf(M_PI * (fu - fl) / s);
	float a2 = a * a;
	float b = tanf(M_PI * (fu - fl) / s);
	float b2 = b * b;
	float r;
	int i;
	for (i = 0; i < filter->n; ++i) {
		r = sinf(M_PI * (2.0 * i + 1.0) / (4.0 * filter->n));
		s = b2 + 2.0 * b * r + 1.0;
		filter->A[i] = b2 / s;
		filter->d1[i] = 4.0 * a * (1.0 + b * r) / s;
		filter->d2[i] = 2.0 * (b2 - 2.0 * a2 - 1.0) / s;
		filter->d3[i] = 4.0 * a * (1.0 - b * r) / s;
		filter->d4[i] = -(b2 - 2.0 * b * r + 1.0) / s;
	}
	return filter;
}
BWBandStop* create_bw_band_stop_filter(int order, float s, float fl, float fu) {
	if (fu <= fl) {
		ESP_LOGE(FILTER_TAG,
				"ERROR:Lower half-power frequency is smaller than higher half-power frequency");
		return NULL;
	}

	BWBandStop* filter = (BWBandStop*) malloc(sizeof(BWBandStop));
	filter->n = order / 4;
	filter->A = (float*) malloc(filter->n * sizeof(float));
	filter->d1 = (float*) malloc(filter->n * sizeof(float));
	filter->d2 = (float*) malloc(filter->n * sizeof(float));
	filter->d3 = (float*) malloc(filter->n * sizeof(float));
	filter->d4 = (float*) malloc(filter->n * sizeof(float));

	filter->w0 = (float*) calloc(filter->n, sizeof(float));
	filter->w1 = (float*) calloc(filter->n, sizeof(float));
	filter->w2 = (float*) calloc(filter->n, sizeof(float));
	filter->w3 = (float*) calloc(filter->n, sizeof(float));
	filter->w4 = (float*) calloc(filter->n, sizeof(float));

	float a = cosf(M_PI * (fu + fl) / s) / cosf(M_PI * (fu - fl) / s);
	float a2 = a * a;
	float b = tanf(M_PI * (fu - fl) / s);
	float b2 = b * b;
	float r;

	int i;
	for (i = 0; i < filter->n; ++i) {
		r = sinf(M_PI * (2.0 * i + 1.0) / (4.0 * filter->n));
		s = b2 + 2.0 * b * r + 1.0;
		filter->A[i] = 1.0 / s;
		filter->d1[i] = 4.0 * a * (1.0 + b * r) / s;
		filter->d2[i] = 2.0 * (b2 - 2.0 * a2 - 1.0) / s;
		filter->d3[i] = 4.0 * a * (1.0 - b * r) / s;
		filter->d4[i] = -(b2 - 2.0 * b * r + 1.0) / s;
	}
	filter->r = 4.0 * a;
	filter->s = 4.0 * a2 + 2.0;
	return filter;
}

void free_bw_low_pass(BWLowPass* filter) {
	if (NULL == filter) return;
	free(filter->A);
	free(filter->d1);
	free(filter->d2);
	free(filter->w0);
	free(filter->w1);
	free(filter->w2);
	free(filter);
}
void free_bw_high_pass(BWHighPass* filter) {
	if (NULL == filter) return;
	free(filter->A);
	free(filter->d1);
	free(filter->d2);
	free(filter->w0);
	free(filter->w1);
	free(filter->w2);
	free(filter);
}
void free_bw_band_pass(BWBandPass* filter) {
	if (NULL == filter) return;
	free(filter->A);
	free(filter->d1);
	free(filter->d2);
	free(filter->d3);
	free(filter->d4);
	free(filter->w0);
	free(filter->w1);
	free(filter->w2);
	free(filter->w3);
	free(filter->w4);
	free(filter);
}
void free_bw_band_stop(BWBandStop* filter) {
	if (NULL == filter) return;
	free(filter->A);
	free(filter->d1);
	free(filter->d2);
	free(filter->d3);
	free(filter->d4);
	free(filter->w0);
	free(filter->w1);
	free(filter->w2);
	free(filter->w3);
	free(filter->w4);
	free(filter);
}

float bw_low_pass(BWLowPass* filter, float x) {
	int i;
	for (i = 0; i < filter->n; ++i) {
		filter->w0[i] = filter->d1[i] * filter->w1[i]
				+ filter->d2[i] * filter->w2[i] + x;
		x = filter->A[i]
				* (filter->w0[i] + 2.0 * filter->w1[i] + filter->w2[i]);
		filter->w2[i] = filter->w1[i];
		filter->w1[i] = filter->w0[i];
	}
	return x;
}

float bw_high_pass(BWHighPass* filter, float x) {
	int i;
	for (i = 0; i < filter->n; ++i) {
		filter->w0[i] = filter->d1[i] * filter->w1[i]
				+ filter->d2[i] * filter->w2[i] + x;
		x = filter->A[i]
				* (filter->w0[i] - 2.0 * filter->w1[i] + filter->w2[i]);
		filter->w2[i] = filter->w1[i];
		filter->w1[i] = filter->w0[i];
	}
	return x;
}

float bw_band_pass(BWBandPass* filter, float x) {
	int i;
	for (i = 0; i < filter->n; ++i) {
		filter->w0[i] = filter->d1[i] * filter->w1[i]
				+ filter->d2[i] * filter->w2[i] + filter->d3[i] * filter->w3[i]
				+ filter->d4[i] * filter->w4[i] + x;
		x = filter->A[i]
				* (filter->w0[i] - 2.0 * filter->w2[i] + filter->w4[i]);
		filter->w4[i] = filter->w3[i];
		filter->w3[i] = filter->w2[i];
		filter->w2[i] = filter->w1[i];
		filter->w1[i] = filter->w0[i];
	}
	return x;
}

float bw_band_stop(BWBandStop* filter, float x) {
	int i;
	for (i = 0; i < filter->n; ++i) {
		filter->w0[i] = filter->d1[i] * filter->w1[i]
				+ filter->d2[i] * filter->w2[i] + filter->d3[i] * filter->w3[i]
				+ filter->d4[i] * filter->w4[i] + x;
		x = filter->A[i]
				* (filter->w0[i] - filter->r * filter->w1[i]
						+ filter->s * filter->w2[i] - filter->r * filter->w3[i]
						+ filter->w4[i]);
		filter->w4[i] = filter->w3[i];
		filter->w3[i] = filter->w2[i];
		filter->w2[i] = filter->w1[i];
		filter->w1[i] = filter->w0[i];
	}
	return x;
}

#endif /* MAIN_FILTER_H_ */
