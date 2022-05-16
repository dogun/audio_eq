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
#include "common_config.h"
#include "i2s_config.h"
#include "eq.h"

typedef unsigned int size_t;

#define BUF_SIZE 1024
static int32_t data[BUF_SIZE];

static SemaphoreHandle_t read_sem;
static SemaphoreHandle_t write_l_sem;
static SemaphoreHandle_t write_r_sem;
static SemaphoreHandle_t eq_l_sem;
static SemaphoreHandle_t eq_r_sem;

void read_task() {
	size_t bytes_read = 0;
	while (true) {
		xSemaphoreTake(read_sem, portMAX_DELAY);
		i2s_read(i2s_num, (char*) data, BUF_SIZE * 4, &bytes_read, 100);
		xSemaphoreGive(eq_r_sem);
		xSemaphoreGive(eq_l_sem);
	}
}

void write_task() {
	size_t bytes_write = 0;
	while (true) {
		xSemaphoreTake(write_l_sem, portMAX_DELAY);
		xSemaphoreTake(write_r_sem, portMAX_DELAY);
		i2s_write(i2s_num, (char*) data, BUF_SIZE * 4, &bytes_write, 100);
		xSemaphoreGive(read_sem);
	}
}

void eq_l_task() {
	while (true) {
		xSemaphoreTake(eq_l_sem, portMAX_DELAY);
		_apply_biquads_l(data, data, BUF_SIZE);
		xSemaphoreGive(write_l_sem);
	}
}

void eq_r_task() {
	while (true) {
		xSemaphoreTake(eq_r_sem, portMAX_DELAY);
		_apply_biquads_r(data, data, BUF_SIZE);
		xSemaphoreGive(write_r_sem);
	}
}

void app_main(void) {

	ESP_LOGI(MAIN_TAG, "init i2s start");
	i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	i2s_set_pin(i2s_num, &pin_config);
	ESP_LOGI(MAIN_TAG, "init i2s end");

	ESP_LOGI(MAIN_TAG, "init eq config");
	_mk_biquad(-7.000000, 42.050000, 10.337000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-5.600000, 51.000000, 5.877000, &(l_biquads[eq_len_l++]));
	_mk_biquad(1.800000, 59.500000, 7.496000, &(l_biquads[eq_len_l++]));
	_mk_biquad(3.600000, 74.600000, 7.498000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-6.400000, 95.100000, 5.288000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-5.900000, 115.000000, 4.664000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-3.100000, 191.500000, 6.663000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-4.500000, 223.000000, 4.490000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-2.600000, 314.000000, 5.046000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-6.000000, 608.000000, 5.530000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-5.200000, 1252.000000, 6.222000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-6.600000, 1374.000000, 6.401000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-4.900000, 1542.000000, 6.309000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-3.400000, 9155.000000, 2.943000, &(l_biquads[eq_len_l++]));
	_mk_biquad(-1.900000, 11612.000000, 2.135000, &(l_biquads[eq_len_l++]));

	_mk_biquad(-6.900000, 42.400000, 10.065000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-6.000000, 51.100000, 6.577000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-2.200000, 67.600000, 13.101000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-8.100000, 82.000000, 4.921000, &(r_biquads[eq_len_r++]));
	_mk_biquad(6.900000, 85.300000, 3.022000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-5.300000, 115.000000, 5.625000, &(r_biquads[eq_len_r++]));
	_mk_biquad(3.000000, 154.000000, 7.441000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-5.500000, 201.000000, 6.717000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-4.200000, 552.000000, 3.447000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-3.800000, 830.000000, 5.167000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-11.000000, 1339.000000, 3.544000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-1.500000, 1934.000000, 2.072000, &(r_biquads[eq_len_r++]));
	_mk_biquad(3.000000, 2328.000000, 5.714000, &(r_biquads[eq_len_r++]));
	_mk_biquad(3.800000, 3317.000000, 2.126000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-3.400000, 4431.000000, 3.536000, &(r_biquads[eq_len_r++]));
	_mk_biquad(3.000000, 6097.000000, 2.744000, &(r_biquads[eq_len_r++]));
	_mk_biquad(-3.900000, 11767.000000, 1.000000, &(r_biquads[eq_len_r++]));
	ESP_LOGI(MAIN_TAG, "eq config ok");

	ESP_LOGI(MAIN_TAG, "start eq task(read write eqx2)");
	read_sem = xSemaphoreCreateBinary();
	write_l_sem = xSemaphoreCreateBinary();
	write_r_sem = xSemaphoreCreateBinary();
	eq_l_sem = xSemaphoreCreateBinary();
	eq_r_sem = xSemaphoreCreateBinary();
	xTaskCreatePinnedToCore(eq_l_task, "eq_task_l", 10000, NULL, 3, NULL, 1);
	xTaskCreatePinnedToCore(eq_r_task, "eq_task_r", 10000, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(write_task, "write_task", 10000, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(read_task, "read_task", 10000, NULL, 3, NULL, 1);
	ESP_LOGI(MAIN_TAG, "eq task ok");

	ESP_LOGI(MAIN_TAG, "give read_sem, start task chain");
	xSemaphoreGive(read_sem);

	vTaskSuspend(NULL);
}

