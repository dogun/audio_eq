#ifndef MAIN_EQUALIZER__H_
#define MAIN_EQUALIZER__H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct {
	float cf;
	float q;
	float gain;
} EQ_CONFIG_ONE;

typedef struct {
	int count;
	EQ_CONFIG_ONE* config;
} EQ_CONFIG;

typedef struct {
	int n;
	float* a0;
	float* a1;
	float* a2;
	float* a3;
	float* a4;
	float* x1;
	float* x2;
	float* y1;
	float* y2;
	float* cf;
	float* q;
	float* gain;
} Equalizer;

Equalizer* create_equalizer(EQ_CONFIG* config, float rate);
void free_equalizer(Equalizer* eq);
float equalizer(Equalizer* eq, float input);

Equalizer* create_equalizer(EQ_CONFIG* config, float rate) {
	if (NULL == config || config->count <= 0) return NULL;

	Equalizer* eq = (Equalizer*)malloc(sizeof(Equalizer));
	eq->n = config->count;
	eq->a0 = (float*)malloc(eq->n * sizeof(float));
	eq->a1 = (float*)malloc(eq->n * sizeof(float));
	eq->a2 = (float*)malloc(eq->n * sizeof(float));
	eq->a3 = (float*)malloc(eq->n * sizeof(float));
	eq->a4 = (float*)malloc(eq->n * sizeof(float));
	eq->cf = (float*)malloc(eq->n * sizeof(float));
	eq->q = (float*)malloc(eq->n * sizeof(float));
	eq->gain = (float*)malloc(eq->n * sizeof(float));

	eq->x1 = (float*)calloc(eq->n, sizeof(float));
	eq->x2 = (float*)calloc(eq->n, sizeof(float));
	eq->y1 = (float*)calloc(eq->n, sizeof(float));
	eq->y2 = (float*)calloc(eq->n, sizeof(float));

	for (int i = 0; i < config->count; i++) {
		eq->cf[i] = config->config[i].cf;
		eq->gain[i] = config->config[i].gain;
		eq->q[i] = config->config[i].q;

		float A = powf(10.0, eq->gain[i] / 40.0);
		float omega = 2.0 * 3.14159265358979323846 * eq->cf[i] / rate;
		float sn = sinf(omega);
		float cs = cosf(omega);
		//float alpha = sn * sinhf(M_LN2 / 2.0f * bw * omega / sn);  //use band width
		float alpha = sn / (2 * eq->q[i]);

		float alpha_m_A = alpha * A;
		float alpha_d_A = alpha / A;

		float b0 = 1.0 + alpha_m_A;
		float b1 = -2.0 * cs;
		float b2 = 1.0 - alpha_m_A;
		float a0 = 1.0 + alpha_d_A;
		float a1 = b1;
		float a2 = 1.0 - alpha_d_A;

		eq->a0[i] = b0 / a0;
		eq->a1[i] = b1 / a0;
		eq->a2[i] = b2 / a0;
		eq->a3[i] = a1 / a0;
		eq->a4[i] = a2 / a0;
	}

	return eq;
}

float equalizer(Equalizer* eq, float input) {
	float output = 0;
	for (int i = 0; i < eq->n; i++) {
		output = input * eq->a0[i] + eq->a1[i] * eq->x1[i]
				+ eq->a2[i] * eq->x2[i] - eq->a3[i] * eq->y1[i]
				- eq->a4[i] * eq->y2[i];
		eq->x2[i] = eq->x1[i];
		eq->x1[i] = input;
		eq->y2[i] = eq->y1[i];
		eq->y1[i] = output;
	}
	return output;
}

void free_equalizer(Equalizer* eq) {
	if (NULL == eq) return;
	free(eq->a0);
	free(eq->a1);
	free(eq->a2);
	free(eq->a3);
	free(eq->a4);
	free(eq->x1);
	free(eq->x2);
	free(eq->y1);
	free(eq->y2);
	free(eq->cf);
	free(eq->gain);
	free(eq->q);
	free(eq);
}

#endif
