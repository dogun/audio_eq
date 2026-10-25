/*
 * eq_config.h
 *
 *  Created on: 2022��5��17��
 *      Author: yuexq
 */

#ifndef MAIN_EQ_CONFIG_H_
#define MAIN_EQ_CONFIG_H_

#include <stdio.h>
#include "config_store.h"
#include "eq.h"

#define CONFIG_SIZE 3900

#define EQ_KEY_L "eq_l"
#define EQ_KEY_R "eq_r"
#define SWAP_KEY "swap_lr"
#define LOUD_KEY "loudness"

/* Set by the HTTP handlers after a config save; consumed by the audio task. */
static volatile int eq_reload_requested = 0;

void _load_eq_new(char* buf, int ch) {
	char* tk;
	char* bbf = buf;
	while ((tk = strsep(&bbf, "\n")) != NULL) {
		if (strlen(tk) == 0)
			continue;
		if (strstr(tk, "None") != NULL)
			continue;

		if (!strstr(tk, "Fc") || !strstr(tk, "Gain") || !strstr(tk, "Q")) {
			char* end = NULL;
			double delay = strtod(tk, &end);
			while (*end == ' ' || *end == '\t' || *end == '\r') end++;
			if (end != tk && *end == '\0') {
				if (ch == L_CODE) {
					l_delay = delay;
					ESP_LOGD(EQ_TAG, "l delay: %f", delay);
				} else {
					r_delay = delay;
					ESP_LOGD(EQ_TAG, "r delay: %f", delay);
				}
			} else {
				ESP_LOGD(EQ_TAG, "_load_eq_new error: %s", tk);
			}
			continue;
		}

		char* token = strtok(tk, " ");
		double cf = 0, gain = 0, q = 0;
		int i = 0;
		while (token != NULL) {
			if (i == 5) cf = strtod(token, NULL);
			else if (i == 8) gain = strtod(token, NULL);
			else if (i == 11) q = strtod(token, NULL);
			i++;
			token = strtok(NULL, " ");
		}
		if (i == 1) { //只有一个数字
			if (ch == L_CODE) {
				l_delay = cf;
				ESP_LOGD(EQ_TAG, "l delay: %f", cf);
			}else {
				r_delay = cf;
				ESP_LOGD(EQ_TAG, "r delay: %f", cf);
			}
		} else if(i > 11) {
			if (ch == L_CODE) {
				if (eq_len_l >= MAX_EQ_COUNT)
					continue;
				if (_mk_biquad(gain, cf, q, &(l_biquads[eq_len_l])))
					eq_len_l++;
			} else {
				if (eq_len_r >= MAX_EQ_COUNT)
					continue;
				if (_mk_biquad(gain, cf, q, &(r_biquads[eq_len_r])))
					eq_len_r++;
			}
		} else {
			ESP_LOGD(EQ_TAG, "_load_eq_new error: %s", tk);
		}
	}
}

void _load_eq_old(char* buf, int ch) {
	char* tk;
	char* bbf = buf;
	while ((tk = strsep(&bbf, "\n")) != NULL) {
		if (strlen(tk) == 0)
			continue;

		char* token;
		double cf = 0, gain = 0, q = 0;
		int i = 0;
		while ((token = strsep(&tk, " ")) != NULL) {
			//ESP_LOGE(EQ_TAG, "token: %s, token1: %d", token, token[0]);
			if (token[0] == ' ' || token[0] == 0) {
				continue;
			}
			if (i == 0)
				cf = strtod(token, NULL);
			else if (i == 1)
				gain = strtod(token, NULL);
			else if (i == 2)
				q = strtod(token, NULL);
			i++;
		}
		if (i == 1) { //只有一个数字
			if (ch == L_CODE) {
				l_delay = cf;
				ESP_LOGD(EQ_TAG, "l delay: %f", cf);
			}else {
				r_delay = cf;
				ESP_LOGD(EQ_TAG, "r delay: %f", cf);
			}
		} else if (i == 3) {
			if (ch == L_CODE) {
				if (eq_len_l >= MAX_EQ_COUNT)
					continue;
				if (_mk_biquad(gain, cf, q, &(l_biquads[eq_len_l])))
					eq_len_l++;
			} else {
				if (eq_len_r >= MAX_EQ_COUNT)
					continue;
				if (_mk_biquad(gain, cf, q, &(r_biquads[eq_len_r])))
					eq_len_r++;
			}
		} else {
			ESP_LOGD(EQ_TAG, "_load_eq_old error: %s", tk);
		}
	}
}

void _load_eq(int ch) {
	char* buf = calloc(CONFIG_SIZE, 1);
	if (buf == NULL) {
		ESP_LOGE(EQ_TAG, "cannot allocate EQ config buffer");
		return;
	}
	read_config(ch == L_CODE ? EQ_KEY_L : EQ_KEY_R, buf, CONFIG_SIZE);

	if (strstr(buf, "Gain")) {
		_load_eq_new(buf, ch);
	} else {
		_load_eq_old(buf, ch);
	}
	free(buf);
}

void load_eq() {
	eq_len_l = 0;
	eq_len_r = 0;
	l_delay = 0;
	r_delay = 0;
	_load_eq(L_CODE);
	_load_eq(R_CODE);
	char swap[8] = {0};
	read_config(SWAP_KEY, swap, sizeof(swap));
	channel_swap = (swap[0] != '\0' && atoi(swap) != 0);

	char loud[64] = {0};
	read_config(LOUD_KEY, loud, sizeof(loud));
	int loud_en = 0;
	float loud_bass = LOUDNESS_DEFAULT_BASS_DB;
	float loud_treble = LOUDNESS_DEFAULT_TREBLE_DB;
	float loud_ref = LOUDNESS_DEFAULT_REF_DBFS;
	float loud_range = LOUDNESS_DEFAULT_RANGE_DB;
	float loud_gate = LOUDNESS_DEFAULT_GATE_DBFS;
	if (loud[0] != '\0') {
		char* cursor = loud;
		char* token;
		int field = 0;
		while (field < 6 && (token = strsep(&cursor, ",")) != NULL) {
			switch (field) {
				case 0: loud_en = atoi(token); break;
				case 1: loud_bass = atof(token); break;
				case 2: loud_treble = atof(token); break;
				case 3: loud_ref = atof(token); break;
				case 4: loud_range = atof(token); break;
				case 5: loud_gate = atof(token); break;
			}
			field++;
		}
	}
	if (!isfinite(loud_bass) || loud_bass < 0.0f) loud_bass = 0.0f;
	if (loud_bass > 12.0f) loud_bass = 12.0f;
	if (!isfinite(loud_treble) || loud_treble < 0.0f) loud_treble = 0.0f;
	if (loud_treble > 12.0f) loud_treble = 12.0f;
	if (!isfinite(loud_ref) || loud_ref < -60.0f) loud_ref = -60.0f;
	if (loud_ref > -6.0f) loud_ref = -6.0f;
	if (!isfinite(loud_range) || loud_range < 10.0f) loud_range = 10.0f;
	if (loud_range > 60.0f) loud_range = 60.0f;
	if (!isfinite(loud_gate) || loud_gate < -90.0f) loud_gate = -90.0f;
	if (loud_gate > -40.0f) loud_gate = -40.0f;
	if (loud_gate >= loud_ref) loud_gate = loud_ref - 1.0f;
	loudness_enabled = loud_en != 0;
	loudness_bass_max_db = loud_bass;
	loudness_treble_max_db = loud_treble;
	loudness_ref_db = loud_ref;
	loudness_range_db = loud_range;
	loudness_gate_db = loud_gate;
	_loudness_reset();
}

#endif /* MAIN_EQ_CONFIG_H_ */
