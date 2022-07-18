/*
 * eq_config.h
 *
 *  Created on: 2022Äê5ÔÂ17ÈÕ
 *      Author: yuexq
 */

#ifndef MAIN_EQ_CONFIG_H_
#define MAIN_EQ_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "../valve/equalizer.h"

int _eq_strsep_cnt(char* str, const char* sep) {
	if (NULL == str || NULL == sep) return 0;
	char* n_str = _new_str(str);
	char* tk;
	char* bbf = n_str;
	int count = 0;
	while ((tk = strsep(&bbf, sep)) != NULL) {
		if (strlen(tk) == 0) continue;
		if (!strstr(tk, "Fc") || !strstr(tk, "Gain") || !strstr(tk, "Q"))
			continue;
		count++;
	}
	free(n_str);
	return count;
}

EQ_CONFIG* parse_eq(char* buf) {
	if (NULL == buf) return NULL;
	EQ_CONFIG* config = (EQ_CONFIG*)malloc(sizeof(EQ_CONFIG));
	config->count = _eq_strsep_cnt(buf, "\n");
	config->config = (EQ_CONFIG_ONE*)malloc(sizeof(EQ_CONFIG_ONE) * config->count);

	char* tk;
	char* bbf = buf;
	int eq_i = 0;
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

		config->config[eq_i].cf = cf;
		config->config[eq_i].q = q;
		config->config[eq_i].gain = gain;

		eq_i++;
	}
	return config;
}

void free_eq_config(EQ_CONFIG* config) {
	if (NULL == config) return;
	if (NULL != config->config) free(config->config);
	free(config);
}

#endif /* MAIN_EQ_CONFIG_H_ */
