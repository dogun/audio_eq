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

#define CONFIG_SIZE 1024

#define CONFIG_KEY_L "l_config"
#define CONFIG_KEY_R "r_config"

void _ch2file(int ch, char* config, int size) {
	if (ch == L_CODE) {
		read_config(CONFIG_KEY_L, config, size);
	}else {
		read_config(CONFIG_KEY_R, config, size);
	}
}

void _load_eq_new(char* buf, int ch) {
	char* tk;
	char* bbf = buf;
	while ((tk = strsep(&bbf, "\n")) != NULL) {
		if (strlen(tk) == 0)
			continue;

		if (!strstr(tk, "Fc") || !strstr(tk, "Gain") || !strstr(tk, "Q")) continue;

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
		if (ch == L_CODE) {
			if (eq_len_l >= MAX_EQ_COUNT)
				continue;
			_mk_biquad(gain, cf, q, &(l_biquads[eq_len_l++]));
		} else {
			if (eq_len_r >= MAX_EQ_COUNT)
				continue;
			_mk_biquad(gain, cf, q, &(r_biquads[eq_len_r++]));
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
			if (i == 0)
				cf = strtod(token, NULL);
			else if (i == 1)
				gain = strtod(token, NULL);
			else if (i == 2)
				q = strtod(token, NULL);
			i++;
		}
		if (ch == L_CODE) {
			if (eq_len_l >= MAX_EQ_COUNT)
				continue;
			_mk_biquad(gain, cf, q, &(l_biquads[eq_len_l++]));
		} else {
			if (eq_len_r >= MAX_EQ_COUNT)
				continue;
			_mk_biquad(gain, cf, q, &(r_biquads[eq_len_r++]));
		}
	}
}

void _load_eq(int ch) {
	char buf[CONFIG_SIZE] = {0};
	char config[64] = {0};
	_ch2file(ch, config, sizeof(config));
	read_config(config, buf, CONFIG_SIZE);

	if (strstr(buf, "Gain")) {
		_load_eq_new(buf, ch);
	} else {
		_load_eq_old(buf, ch);
	}
}

void load_eq() {
	_load_eq(L_CODE);
	_load_eq(R_CODE);
}

#endif /* MAIN_EQ_CONFIG_H_ */
