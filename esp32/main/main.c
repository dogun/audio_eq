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
	float vol = powf(10.0f, -3 / 20.0f);

	while (true) {
		xSemaphoreTake(eq_l_sem, portMAX_DELAY);
		_apply_biquads_l(data, data, BUF_SIZE, vol);
		xSemaphoreGive(write_l_sem);
	}
}

void eq_r_task() {
	float vol = powf(10.0f, -3 / 20.0f);

	while (true) {
		xSemaphoreTake(eq_r_sem, portMAX_DELAY);
		_apply_biquads_r(data, data, BUF_SIZE, vol);
		xSemaphoreGive(write_r_sem);
	}
}

void app_main(void) {

	ESP_LOGI(MAIN_TAG, "init i2s start");
	i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	i2s_set_pin(i2s_num, &pin_config);
	ESP_LOGI(MAIN_TAG, "init i2s end");

	ESP_LOGI(MAIN_TAG, "init eq config");
	_mk_biquad(4, 50, 2, &(l_biquads[eq_len_l++]));
	_mk_biquad(-3, 50, 2, &(r_biquads[eq_len_r++]));
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

