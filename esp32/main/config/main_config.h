/*
 * main_config.h
 *
 *  Created on: 2022Äê7ÔÂ14ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_MAIN_CONFIG_H_
#define MAIN_MAIN_CONFIG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "const.h"

typedef struct {
	char* type;
	char* name;
	char** params;
	int param_cnt;
} MAIN_CONFIG_NODE;

typedef struct {
	int count;
	MAIN_CONFIG_NODE* nodes;
} MAIN_CONFIG;

MAIN_CONFIG* parse_main_config(char* config) {
	if (NULL == config) return NULL;
	MAIN_CONFIG* main_config = (MAIN_CONFIG*)malloc(sizeof(MAIN_CONFIG));
	main_config->count = _strsep_cnt(config, "\n");
	main_config->nodes = (MAIN_CONFIG_NODE*)calloc(main_config->count, sizeof(MAIN_CONFIG_NODE));

	char* tmp_config = config;
	char* line;
	int i = 0;
	while ((line = strsep(&tmp_config, "\n")) != NULL) {
		if (strlen(line) == 0) continue;
		int node_sep_count = _strsep_cnt(line, " ");
		if (node_sep_count > 2) {
			main_config->nodes[i].param_cnt = node_sep_count - 2;
			main_config->nodes[i].params = (char**)malloc((main_config->nodes[i].param_cnt) * sizeof(char*));
		} else {
			main_config->nodes[i].param_cnt = 0;
		}
		char* node_tk;
		char* tmp_line = line;
		int j = 0;
		while ((node_tk = strsep(&tmp_line, " ")) != NULL) {
			if (strlen(node_tk) == 0) continue;
			if (j == 0) main_config->nodes[i].type = _new_str(node_tk);
			else if (j == 1) main_config->nodes[i].name = _new_str(node_tk);
			else {
				main_config->nodes[i].params[j - 2] = _new_str(node_tk);
			}
			j++;
		}
		i++;
	}
	return main_config;
}

MAIN_CONFIG_NODE get_config_by_name(MAIN_CONFIG* config, char* name) {
	MAIN_CONFIG_NODE node = {};
	if (NULL == config) {
		return node;
	}
	int i;
	for (i = 0; i < config->count; ++i) {
		char* _name = config->nodes[i].name;
		if (strcmp(name, _name) == 0) {
			return config->nodes[i];
		}
	}
	return node;
}

MAIN_CONFIG get_config_by_type(MAIN_CONFIG* config, char* type) {
	MAIN_CONFIG mc = {.count = 0};
	if (NULL == config) {
		return mc;
	}
	int i;
	for (i = 0; i < config->count; ++i) {
		char* t = config->nodes[i].type;
		if (strcmp(t, type) == 0) {
			mc.count ++;
		}
	}

	if (mc.count > 0) {
		MAIN_CONFIG_NODE nodes[mc.count];
		mc.nodes = nodes;
		int j = 0;
		for (i = 0; i < config->count; ++i) {
			if (strcmp(config->nodes[i].type, type) != 0) continue;
			mc.nodes[j++] = config->nodes[i];
		}
	}
	return mc;
}

void free_main_config(MAIN_CONFIG* config) {
	if (NULL == config) return;
	int i;
	for (i = 0; i < config->count; ++i) {
		MAIN_CONFIG_NODE node = config->nodes[i];
		free(node.name);
		free(node.type);
		int j;
		for (j = 0; j < node.param_cnt; ++j) {
			free(node.params[j]);
		}
		free(node.params);
	}
	free(config->nodes);
	free(config);
}

int _main() {
	char* test = "INPUT_L\nINPUT_R\n\nPREAMP amp1 -1\nEQ eq_l eq_l.conf\nEQ eq_r eq_r.conf\nLOW_PASS lp_l 2 200\nHIGH_PASS hp_r 2 1000\nMAP m_low_l 0.1 1 0.3 0.9\nMAP m_high_l 0.1 1 0.3 0.9\nMAX_VOL mv_1\nOUTPUT_L eq_l INPUT_L\nOUTPUT_R eq_r INPUT_R\nPWM_OUTPUT 18 m_low_l lp_l mv_1 INPUT_L\nPWM_OUTPUT 19 m_high_l hp_r mv_1 INPUT_R\n";
	MAIN_CONFIG* config = parse_main_config(_new_str(test));
	int i;
	for (i = 0; i < config->count; ++i) {
		MAIN_CONFIG_NODE node = config->nodes[i];
		printf("name: %s, type: %s, param_cnt: %d\n", node.name, node.type, node.param_cnt);
		int j;
		for (j = 0; j < node.param_cnt; ++j) {
			printf("    %s\n", node.params[j]);
		}
	}
	free_main_config(config);
	return 0;
}

#endif /* MAIN_MAIN_CONFIG_H_ */
