#ifndef MAIN_EQ__H_
#define MAIN_EQ__H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "audio_sample.h"
#include "common_config.h"

#define MAX_EQ_COUNT 50
#define PREAMPF 0.89125094f

typedef struct t_biquad t_biquad;
struct t_biquad {
	float a0, a1, a2, a3, a4;
	float x1, x2, y1, y2;
	float cf, q, gain;
};

static inline float _apply_biquads_one(float s, t_biquad* b);
static inline void _biquads_x(int32_t* src, int32_t* dst, int len, t_biquad* biquad, int start, int eqline);
static inline void _apply_biquads_r(int32_t* src, int32_t* dst, int len);
static inline void _apply_biquads_l(int32_t* src, int32_t* dst, int len);

static int _mk_biquad(float dbgain, float cf, float q, t_biquad *b);

int eq_len_r;
int eq_len_l;
static t_biquad r_biquads[MAX_EQ_COUNT];
static t_biquad l_biquads[MAX_EQ_COUNT];
static float r_delay = 0;
static float l_delay = 0;
/* When set, eq_l applies to slot 1 and eq_r to slot 0. */
static volatile int channel_swap = 0;

/* Signal-level based loudness compensation (approximation, not absolute volume). */
#define LOUDNESS_BASS_HZ 100.0f
#define LOUDNESS_TREBLE_HZ 10000.0f
#define LOUDNESS_SHELF_Q 0.707f
#define LOUDNESS_DEFAULT_REF_DBFS (-18.0f)
#define LOUDNESS_DEFAULT_RANGE_DB 30.0f
#define LOUDNESS_DEFAULT_GATE_DBFS (-75.0f)
#define LOUDNESS_ATTACK_COEFF 0.25f
#define LOUDNESS_RELEASE_COEFF 0.01f
#define LOUDNESS_DEFAULT_BASS_DB 8.0f
#define LOUDNESS_DEFAULT_TREBLE_DB 4.0f

static volatile int loudness_enabled = 0;
static float loudness_factor = 0.0f;
static float loudness_level_db = -90.0f;
static int loudness_primed = 0;
static float loudness_bass_max_db = LOUDNESS_DEFAULT_BASS_DB;
static float loudness_treble_max_db = LOUDNESS_DEFAULT_TREBLE_DB;
static float loudness_ref_db = LOUDNESS_DEFAULT_REF_DBFS;
static float loudness_range_db = LOUDNESS_DEFAULT_RANGE_DB;
static float loudness_gate_db = LOUDNESS_DEFAULT_GATE_DBFS;
static t_biquad loudness_bass[2];
static t_biquad loudness_treble[2];
static float loudness_last_bass_db = -1.0f;
static float loudness_last_treble_db = -1.0f;

static inline void _biquads_x(int32_t* src, int32_t* dst, int len, t_biquad* biquad, int start, int eqline) {
	int i, di;

	for (i = start; i < len; i += 2) {
		int32_t rl = audio_i2s_to_s24(src[i]);

		float l_s = (float)rl * PREAMPF;
		float l_f = l_s;

		for (di = 0; di < eqline; ++di) {
			l_f = _apply_biquads_one(l_s, &(biquad[di]));
			l_s = l_f;
		}
		if (loudness_enabled && loudness_factor > 0.001f) {
			int slot = i & 1;
			l_f = _apply_biquads_one(l_f, &loudness_bass[slot]);
			l_f = _apply_biquads_one(l_f, &loudness_treble[slot]);
		}
		dst[i] = audio_s24_to_i2s(audio_clip_float_s24(l_f));
	}
}

static inline void _apply_biquads_l(int32_t* src, int32_t* dst, int len) {
	_biquads_x(src, dst, len, l_biquads, channel_swap ? R_CODE : L_CODE, eq_len_l);
}

static inline void _apply_biquads_r(int32_t* src, int32_t* dst, int len) {
	_biquads_x(src, dst, len, r_biquads, channel_swap ? L_CODE : R_CODE, eq_len_r);
}

static inline float _apply_biquads_one(float s, t_biquad* b) {
	float f =
		s * b->a0 \
		+ b->a1 * b->x1 \
		+ b->a2 * b->x2 \
		- b->a3 * b->y1 \
		- b->a4 * b->y2;
	b->x2 = b->x1;
	b->x1 = s;
	b->y2 = b->y1;
	b->y1 = f;
	return f;
}

/* Audio EQ Cookbook low/high shelf coefficients. State is preserved. */
static void _mk_shelf(float gain_db, float cf, float q, int high, t_biquad* b) {
	float A = powf(10.0f, gain_db / 40.0f);
	float w0 = 2.0f * 3.14159265358979323846f * cf / RATE;
	float cw = cosf(w0);
	float sw = sinf(w0);
	float alpha = sw / (2.0f * q);
	float beta = 2.0f * sqrtf(A) * alpha;
	float b0, b1, b2, a0, a1, a2;

	if (!high) {
		b0 = A * ((A + 1.0f) - (A - 1.0f) * cw + beta);
		b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cw);
		b2 = A * ((A + 1.0f) - (A - 1.0f) * cw - beta);
		a0 = (A + 1.0f) + (A - 1.0f) * cw + beta;
		a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cw);
		a2 = (A + 1.0f) + (A - 1.0f) * cw - beta;
	} else {
		b0 = A * ((A + 1.0f) + (A - 1.0f) * cw + beta);
		b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw);
		b2 = A * ((A + 1.0f) + (A - 1.0f) * cw - beta);
		a0 = (A + 1.0f) - (A - 1.0f) * cw + beta;
		a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cw);
		a2 = (A + 1.0f) - (A - 1.0f) * cw - beta;
	}

	b->a0 = b0 / a0;
	b->a1 = b1 / a0;
	b->a2 = b2 / a0;
	b->a3 = a1 / a0;
	b->a4 = a2 / a0;
	b->cf = cf;
	b->q = q;
	b->gain = gain_db;
}

static inline void _loudness_clear_state(void) {
	for (int i = 0; i < 2; ++i) {
		loudness_bass[i].x1 = loudness_bass[i].x2 = 0.0f;
		loudness_bass[i].y1 = loudness_bass[i].y2 = 0.0f;
		loudness_treble[i].x1 = loudness_treble[i].x2 = 0.0f;
		loudness_treble[i].y1 = loudness_treble[i].y2 = 0.0f;
	}
}

static inline void _loudness_reset(void) {
	loudness_factor = 0.0f;
	loudness_level_db = -90.0f;
	loudness_primed = 0;
	loudness_last_bass_db = -1.0f;
	loudness_last_treble_db = -1.0f;
	memset(loudness_bass, 0, sizeof(loudness_bass));
	memset(loudness_treble, 0, sizeof(loudness_treble));
}

static inline void loudness_update(int32_t* src, int len) {
	if (!loudness_enabled) {
		loudness_factor = 0.0f;
		loudness_primed = 0;
		return;
	}

	int64_t sumsq = 0;
	for (int i = 0; i < len; ++i) {
		int32_t s = audio_i2s_to_s24(src[i]);
		sumsq += (int64_t)s * s;
	}
	float rms = sqrtf((float)sumsq / (float)len);
	float db = rms > 0.0f ? 20.0f * log10f(rms / 8388608.0f) : -90.0f;
	if (!loudness_primed) {
		loudness_level_db = db;
		loudness_primed = 1;
	} else {
		float alpha = db > loudness_level_db ? LOUDNESS_ATTACK_COEFF : LOUDNESS_RELEASE_COEFF;
		loudness_level_db += (db - loudness_level_db) * alpha;
	}

	float target = (loudness_ref_db - loudness_level_db) / loudness_range_db;
	if (loudness_level_db < loudness_gate_db) target = 0.0f;
	if (target < 0.0f) target = 0.0f;
	if (target > 1.0f) target = 1.0f;

	float previous = loudness_factor;
	float factor_alpha = target < loudness_factor ? 0.4f : 0.1f;
	loudness_factor += (target - loudness_factor) * factor_alpha;
	if (loudness_factor < 0.0001f) {
		loudness_factor = 0.0f;
		if (previous > 0.0f) _loudness_clear_state();
	}

	float bass_db = loudness_factor * loudness_bass_max_db;
	float treble_db = loudness_factor * loudness_treble_max_db;
	if (fabsf(bass_db - loudness_last_bass_db) >= 0.05f ||
			fabsf(treble_db - loudness_last_treble_db) >= 0.05f) {
		for (int slot = 0; slot < 2; ++slot) {
			_mk_shelf(bass_db, LOUDNESS_BASS_HZ, LOUDNESS_SHELF_Q, 0, &loudness_bass[slot]);
			_mk_shelf(treble_db, LOUDNESS_TREBLE_HZ, LOUDNESS_SHELF_Q, 1, &loudness_treble[slot]);
		}
		loudness_last_bass_db = bass_db;
		loudness_last_treble_db = treble_db;
	}
}


/* biquad functions */

/* Create a Peaking EQ Filter.
 * See 'Audio EQ Cookbook' for more information
 */
static int _mk_biquad(float dbgain, float cf, float q, t_biquad* b) {
  if (b == NULL || !isfinite(dbgain) || !isfinite(cf) || !isfinite(q)
		  || cf <= 0.0f || cf >= RATE / 2.0f
		  || q <= 0.0f || q > 1000.0f
		  || dbgain < -120.0f || dbgain > 120.0f) {
	  ESP_LOGE(EQ_TAG, "invalid biquad: cf=%f gain=%f q=%f", cf, dbgain, q);
	  return 0;
  }

  ESP_LOGD(EQ_TAG, "make biquad: %f %f %f", cf, dbgain, q);
  float A = powf(10.0, dbgain / 40.0);
  float omega = 2.0 * 3.14159265358979323846 * cf / RATE;
  float sn = sinf(omega);
  float cs = cosf(omega);
  //float alpha = sn * sinh(M_LN2 / 2.0f * bw * omega / sn);  //use band width
  float alpha = sn / (2 * q);

  float alpha_m_A = alpha * A;
  float alpha_d_A = alpha / A;

  float b0 = 1.0 + alpha_m_A;
  float b1 = -2.0 * cs;
  float b2 = 1.0 - alpha_m_A;
  float a0 = 1.0 + alpha_d_A;
  float a1 = b1;
  float a2 = 1.0 - alpha_d_A;

  b->a0 = b0 / a0;
  b->a1 = b1 / a0;
  b->a2 = b2 / a0;
  b->a3 = a1 / a0;
  b->a4 = a2 / a0;

  b->x1 = 0.0;
  b->x2 = 0.0;
  b->y1 = 0.0;
  b->y2 = 0.0;

  b->cf = cf;
  b->q = q;
  b->gain = dbgain;
  return 1;
}

#endif
