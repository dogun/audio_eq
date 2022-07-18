/*
 * pipline.h
 *
 *  Created on: 2022Äê7ÔÂ16ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_OUTPUT_PIPLINE_H_
#define MAIN_OUTPUT_PIPLINE_H_

#include <stdlib.h>
#include <stdio.h>
#include "../config/main_config.h"
#include "buffer.h"

#define OUTPUT_CONFIG_TAG "OUTPUT"

typedef struct {
	char* output_type;
	char* output_name;
	int node_count;
	MAIN_CONFIG_NODE* node_chain;
	DATA_BUFFER* buffer;
} OUTPUT_PIPLINE;

typedef struct {
	int count;
	OUTPUT_PIPLINE* piplines;
} OUTPUT_PIPLINE_LIST;

OUTPUT_PIPLINE_LIST* make_pipline_list(MAIN_CONFIG* config) {
	if (NULL == config) return NULL;
	OUTPUT_PIPLINE_LIST* pl = (OUTPUT_PIPLINE_LIST*)malloc(sizeof(OUTPUT_PIPLINE_LIST));
	pl->count = 0;
	int i;
	for (i = 0; i < config->count; ++i) {
		MAIN_CONFIG_NODE node = config->nodes[i];
		if (strstr(node.type, OUTPUT_CONFIG_TAG)) {
			pl->count++;
		}
	}
	MAIN_CONFIG_NODE buf_size_node = get_config_by_name(config, BUF_SIZE_CONFIG);
	int buf_size = 256;
	if (buf_size_node.param_cnt > 0) buf_size = atoi(buf_size_node.params[0]);

	pl->piplines = (OUTPUT_PIPLINE*)malloc(pl->count * sizeof(OUTPUT_PIPLINE));
	int j = 0;
	for (i = 0; i < config->count; ++i) {
		MAIN_CONFIG_NODE node = config->nodes[i];
		if (strstr(node.type, OUTPUT_CONFIG_TAG)) {
			pl->piplines[j].buffer = (DATA_BUFFER*)malloc(sizeof(DATA_BUFFER));
			pl->piplines[j].buffer->len = buf_size;
			pl->piplines[j].buffer->buf = (int32_t*)malloc(buf_size * sizeof(int32_t));
			pl->piplines[j].output_name = node.name;
			pl->piplines[j].output_type = node.type;
			pl->piplines[j].node_count = node.param_cnt;
			pl->piplines[j].node_chain = (MAIN_CONFIG_NODE*)malloc(node.param_cnt * sizeof(MAIN_CONFIG_NODE));
			int x;
			for (x = 0; x < node.param_cnt; ++x) {
				char* p = node.params[x];
				MAIN_CONFIG_NODE n = get_config_by_name(config, p);
				pl->piplines[j].node_chain[x] = n;
			}
			j++;
		}
	}
	return pl;
}

void free_pipline_list(OUTPUT_PIPLINE_LIST* pl) {
	if (NULL == pl) return;
	int i;
	for (i = 0; i < pl->count; ++i) {
		free(pl->piplines[i].node_chain);
		free(pl->piplines[i].buffer->buf);
		free(pl->piplines[i].buffer);
	}
	free(pl->piplines);
	free(pl);
}

#endif /* MAIN_OUTPUT_PIPLINE_H_ */
