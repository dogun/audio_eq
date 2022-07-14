/*
 * eq_config.h
 *
 *  Created on: 2022Äê5ÔÂ17ÈÕ
 *      Author: yuexq
 */

#ifndef MAIN_EQ_CONFIG_H_
#define MAIN_EQ_CONFIG_H_

#include <stdio.h>
#include "config_store.h"
#include "eq.h"

#define CONFIG_SIZE 2048

#define CONFIG_KEY_L "l_config"
#define CONFIG_KEY_R "r_config"

char* _ch2key(int ch) {
	if (ch == L_CODE) {
		return CONFIG_KEY_L;
	}else {
		return CONFIG_KEY_R;
	}
}

void _load_eq(int ch) {
	char buf[CONFIG_SIZE] = {0};
	read_config(_ch2key(ch), buf, CONFIG_SIZE);

	char* tk;
	char* bbf = buf;
	while ((tk = strsep(&bbf, "\n")) != NULL) {
		if (strlen(tk) == 0)
			continue;
		if (!strstr(tk, "Fc") || !strstr(tk, "Gain") || !strstr(tk, "Q"))
			continue;

		char* token = strtok(tk, " ");
		float cf = 0, gain = 0, q = 0;
		int i = 0;
		while (token != NULL) {
			if (i == 5) cf = strtof(token, NULL);
			else if (i == 8) gain = strtof(token, NULL);
			else if (i == 11) q = strtof(token, NULL);
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

void load_eq() {
	_load_eq(L_CODE);
	_load_eq(R_CODE);
}

#endif /* MAIN_EQ_CONFIG_H_ */
