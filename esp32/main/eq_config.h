/*
 * eq_config.h
 *
 *  Created on: 2022Äê5ÔÂ17ÈÕ
 *      Author: yuexq
 */

#ifndef MAIN_EQ_CONFIG_H_
#define MAIN_EQ_CONFIG_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "eq.h"

typedef unsigned int size_t;

#define STORAGE_NAMESPACE "storage"
#define CONFIG_SIZE 1024

#define L_NAME "ltxt"
#define L_CODE 0
#define R_NAME "rtxt"
#define R_CODE 1

void init_fs() {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES
			|| err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK (nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(MAIN_TAG, "erase nvs flash and init");
	}
	ESP_ERROR_CHECK(err);
}

nvs_handle_t _open_nvs() {
	nvs_handle_t my_handle;
	esp_err_t err;
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(EQ_TAG, "open nvs error:%d", err);
		return 0;
	}
	return my_handle;
}

char* _ch2file(int ch) {
	char *f = L_NAME;
	if (ch != L_CODE)
		f = R_NAME;
	return f;
}

void read_eq(int ch, char *buf, int size) {
	nvs_handle_t my_handle = _open_nvs();

	char *f = _ch2file(ch);
	size_t len = size;
	ESP_LOGI(MAIN_TAG, "read data %s", f);
	esp_err_t err = nvs_get_str(my_handle, f, buf, &len);
	if (err != ESP_OK || len == 0) {
		ESP_LOGE(EQ_TAG, "get data error:%s, %s, %d", f, esp_err_to_name(err), len);
		nvs_close(my_handle);
		return;
	}
	nvs_close(my_handle);
}

void _load_eq(int ch) {
	char buf[CONFIG_SIZE] = {0};
	read_eq(ch, buf, CONFIG_SIZE);

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


void load_eq() {
	_load_eq(L_CODE);
	_load_eq(R_CODE);
}

void save_eq(int ch, char *config) {
	char *file = _ch2file(ch);

	nvs_handle_t my_handle = _open_nvs();
	ESP_LOGI(MAIN_TAG, "write data %s %s", file, config);
	esp_err_t err = nvs_set_str(my_handle, file, config);
	if (err != ESP_OK) {
		ESP_LOGE(EQ_TAG, "set data error:%s, %d", file, err);
		nvs_close(my_handle);
		return;
	}
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(EQ_TAG, "commit data error:%s, %d", file, err);
		nvs_close(my_handle);
		return;
	}
	nvs_close(my_handle);
}

#endif /* MAIN_EQ_CONFIG_H_ */
