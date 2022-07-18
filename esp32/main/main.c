#include <esp_err.h>
#include <esp_event_legacy.h>
#include <esp_intr_alloc.h>
#include <math.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/_stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#include <freertos/semphr.h>
#include <driver/touch_pad.h>

#include "config/eq_config.h"
#include "config/main_config.h"

#include "esp/i2s_config.h"
#include "esp/wifi.h"
#include "esp/http_op.h"
#include "esp/pwmout.h"
#include "esp/config_store.h"

#include "valve/filter.h"
#include "valve/equalizer.h"
#include "valve/max_val.h"
#include "valve/val_map.h"

#include "output/pipline.h"
#include "output/buffer.h"

typedef unsigned int size_t;

#define MAIN_TAG "main"
#define BUF_SIZE 256
static int32_t data[BUF_SIZE];

static SemaphoreHandle_t read_sem;
static SemaphoreHandle_t write_l_sem;
static SemaphoreHandle_t write_r_sem;
static SemaphoreHandle_t proccess_l_sem;
static SemaphoreHandle_t proccess_r_sem;

void pre_task(int32_t* src, int32_t* dst, int len) {

}

void post_task(int32_t* src, int32_t* dst, int len) {
	//if (maxvol_config == 1) maxvol(src, dst, len, L_CODE, up_start);
}

void read_task() {
	//size_t bytes_read = 0;
	while (true) {
		xSemaphoreTake(read_sem, portMAX_DELAY);
		//i2s_read(i2s_num, (char*) data, BUF_SIZE * 4, &bytes_read, 100);
		pre_task(data, data, BUF_SIZE);
		xSemaphoreGive(proccess_r_sem);
		xSemaphoreGive(proccess_l_sem);
	}
}

void write_task() {
	//size_t bytes_write = 0;
	while (true) {
		xSemaphoreTake(write_l_sem, portMAX_DELAY);
		xSemaphoreTake(write_r_sem, portMAX_DELAY);
		post_task(data, data, BUF_SIZE);
		//i2s_write(i2s_num, (char*) data, BUF_SIZE * 4, &bytes_write, 100);
		xSemaphoreGive(read_sem);
	}
}

void p_l_task() {
	while (true) {
		xSemaphoreTake(proccess_l_sem, portMAX_DELAY);
		//_apply_biquads_l(data, data, BUF_SIZE);
		xSemaphoreGive(write_l_sem);
	}
}

void p_r_task() {
	while (true) {
		xSemaphoreTake(proccess_r_sem, portMAX_DELAY);
		//_apply_biquads_r(data, data, BUF_SIZE);
		xSemaphoreGive(write_r_sem);
	}
}

void _print_main_config(MAIN_CONFIG* config) {
	int i;
	for (i = 0; i < config->count; ++i) {
		if (NULL == config->nodes[i].name) {
			ESP_LOGE(MAIN_TAG, "main config name NULL");
		} else if (NULL == config->nodes[i].type) {
			ESP_LOGE(MAIN_TAG, "main config type NULL");
		} else {
			ESP_LOGI(MAIN_TAG, "main config name:%s, type:%s, param_cnt:%d", config->nodes[i].name, config->nodes[i].type, config->nodes[i].param_cnt);
		}
		int j;
		for (j = 0; j < config->nodes[i].param_cnt; ++j) {
			if (NULL == config->nodes[i].params[j]) {
				ESP_LOGE(MAIN_TAG, "    param NULL: %d", j);
			} else {
				ESP_LOGI(MAIN_TAG, "    param%d:%s", j, config->nodes[i].params[j]);
			}
		}
	}
}

void _print_pipline(OUTPUT_PIPLINE_LIST* list) {
	int i;
	for (i = 0; i < list->count; ++i) {
		OUTPUT_PIPLINE p = list->piplines[i];
		if (NULL == p.output_name) {
			ESP_LOGE(MAIN_TAG, "pipline name NULL");
		} else if (NULL == p.output_type) {
			ESP_LOGE(MAIN_TAG, "pipline type NULL");
		} else {
			ESP_LOGI(MAIN_TAG, "pipline valve cnt:%d, name:%s, type:%s", p.node_count, p.output_name, p.output_type);
		}
		int j;
		for (j = 0; j < p.node_count; ++j) {
			MAIN_CONFIG_NODE n = p.node_chain[j];
			if (NULL == n.name) {
				ESP_LOGE(MAIN_TAG, "    node config name NULL");
			} else if (NULL == n.type) {
				ESP_LOGE(MAIN_TAG, "    node config type NULL");
			} else {
				ESP_LOGI(MAIN_TAG, "    node config name:%s, type:%s, param_cnt:%d", n.name, n.type, n.param_cnt);
			}
			int x;
			for (x = 0; x < n.param_cnt; ++x) {
				if (NULL == n.params[x]) {
					ESP_LOGE(MAIN_TAG, "        param NULL: %d", j);
				} else {
					ESP_LOGI(MAIN_TAG, "        param%d:%s", x, n.params[x]);
				}
			}
		}
	}
}

void app_main(void) {
	ESP_LOGI(MAIN_TAG, "start config fs");
	init_fs();
	ESP_LOGI(MAIN_TAG, "config fs finished");

	ESP_LOGI(MAIN_TAG, "start parse main config");
	int config_len = read_config_len(CONFIG_KEY);
	char* config_str = (char*)calloc(config_len + 1, sizeof(char));
	read_config(CONFIG_KEY, config_str, config_len + 1);
	MAIN_CONFIG* config = parse_main_config(config_str);
	free(config_str);
	ESP_LOGI(MAIN_TAG, "main config: %d", config->count);
	_print_main_config(config);

	ESP_LOGI(MAIN_TAG, "start init softap");
	init_wifi_softap(config);
	ESP_LOGI(MAIN_TAG, "init softap finished");

	ESP_LOGI(MAIN_TAG, "start create pipline");
	OUTPUT_PIPLINE_LIST* plist = make_pipline_list(config);
	ESP_LOGI(MAIN_TAG, "pipline len: %d", plist->count);
	_print_pipline(plist);

	free_pipline_list(plist);
	free_main_config(config);

	//TODO
	ESP_LOGI(MAIN_TAG, "start p task(read write eqx2)");
	read_sem = xSemaphoreCreateBinary();
	write_l_sem = xSemaphoreCreateBinary();
	write_r_sem = xSemaphoreCreateBinary();
	proccess_l_sem = xSemaphoreCreateBinary();
	proccess_r_sem = xSemaphoreCreateBinary();
	xTaskCreatePinnedToCore(&p_l_task, "p_task_l", 10000, NULL, 3, NULL, 1);
	xTaskCreatePinnedToCore(&p_r_task, "p_task_r", 10000, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(&write_task, "write_task", 10000, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(&read_task, "read_task", 10000, NULL, 3, NULL, 1);
	ESP_LOGI(MAIN_TAG, "p task ok");

	ESP_LOGI(MAIN_TAG, "give read_sem, start task chain");
	xSemaphoreGive(read_sem);

	ESP_ERROR_CHECK(touch_pad_init());
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	touch_pad_config(TOUCH_PAD_NUM8, 0);

	uint16_t touch_value;
	int http_started = 0;
	httpd_handle_t server;
	while (1) {
		touch_pad_read(TOUCH_PAD_NUM8, &touch_value);
		if(touch_value < 500) {
			ESP_LOGI(MAIN_TAG, "T:[%d] ", touch_value);
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			touch_pad_read(TOUCH_PAD_NUM8, &touch_value);
			if(touch_value < 500) {
				if (http_started == 0) {
					ESP_LOGI(MAIN_TAG, "start http server");
					ESP_ERROR_CHECK(esp_wifi_start());
					server = start_webserver();
					http_started = 1;
				}else {
					stop_webserver(server);
					esp_wifi_stop();
					http_started = 0;
				}
				vTaskDelay(3000 / portTICK_PERIOD_MS);
			}
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	//vTaskSuspend(NULL);
	free_main_config(config);
}
