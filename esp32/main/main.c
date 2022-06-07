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
#include "driver/touch_pad.h"
#include "common_config.h"
#include "eq_config.h"
#include "i2s_config.h"
#include "eq.h"
#include "http_op.h"
#include "stereo2mono.h"
#include "maxvol.h"

typedef unsigned int size_t;

#define BUF_SIZE 1024
static int32_t data[BUF_SIZE];
static int stereo2mono_config = 0;
static int maxvol_config = 0;

static SemaphoreHandle_t read_sem;
static SemaphoreHandle_t write_l_sem;
static SemaphoreHandle_t write_r_sem;
static SemaphoreHandle_t eq_l_sem;
static SemaphoreHandle_t eq_r_sem;

void pre_task(int32_t* src, int32_t* dst, int len) {
	if (stereo2mono_config == 1) stereo2mono(src, dst, len);
}

void post_task(int32_t* src, int32_t* dst, int len) {
	if (maxvol_config == 1) maxvol(src, dst, len, R_CODE);
}

void read_task() {
	size_t bytes_read = 0;
	while (true) {
		xSemaphoreTake(read_sem, portMAX_DELAY);
		i2s_read(i2s_num, (char*) data, BUF_SIZE * 4, &bytes_read, 100);
		pre_task(data, data, BUF_SIZE);
		xSemaphoreGive(eq_r_sem);
		xSemaphoreGive(eq_l_sem);
	}
}

void write_task() {
	size_t bytes_write = 0;
	while (true) {
		xSemaphoreTake(write_l_sem, portMAX_DELAY);
		xSemaphoreTake(write_r_sem, portMAX_DELAY);
		post_task(data, data, BUF_SIZE);
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

#define SSID "ESP32-AP"
#define PASS "123456789"
#define MAX_CON 5

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data) {
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event =
				(wifi_event_ap_staconnected_t*) event_data;
		ESP_LOGI(MAIN_TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac),
				event->aid);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event =
				(wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGI(MAIN_TAG, "station "MACSTR" leave, AID=%d",
				MAC2STR(event->mac), event->aid);
	}
}

void wifi_init_softap(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

	wifi_config_t wifi_config = { .ap = { .ssid = SSID, .ssid_len = strlen(
	SSID), .password = PASS, .max_connection =
	MAX_CON, .authmode = WIFI_AUTH_WPA_WPA2_PSK } };
	if (strlen(PASS) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
}

void app_main(void) {
	ESP_LOGI(MAIN_TAG, "config fs");
	init_fs();
	ESP_LOGI(MAIN_TAG, "config fs ok");

	ESP_LOGI(MAIN_TAG, "read eq config");
	load_eq();
	ESP_LOGI(MAIN_TAG, "read eq config ok");

	ESP_LOGI(MAIN_TAG, "read stereo2mono config");
	char s2m[16] = {0};
	read_config("s2m", s2m, sizeof(s2m));
	if (strlen(s2m) > 0) stereo2mono_config = atoi(s2m);
	ESP_LOGI(MAIN_TAG, "read stereo2mono config: %s", s2m);

	ESP_LOGI(MAIN_TAG, "read stereo2mono config");
	char maxvol[16] = {0};
	read_config("maxvol", maxvol, sizeof(maxvol));
	if (strlen(maxvol) > 0) maxvol_config = atoi(maxvol);
	ESP_LOGI(MAIN_TAG, "read maxvol config: %s", maxvol);

	ESP_LOGI(MAIN_TAG, "init i2s start");
	i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	i2s_set_pin(i2s_num, &pin_config);
	ESP_LOGI(MAIN_TAG, "init i2s end");

	ESP_LOGI(MAIN_TAG, "start eq task(read write eqx2)");
	read_sem = xSemaphoreCreateBinary();
	write_l_sem = xSemaphoreCreateBinary();
	write_r_sem = xSemaphoreCreateBinary();
	eq_l_sem = xSemaphoreCreateBinary();
	eq_r_sem = xSemaphoreCreateBinary();
	xTaskCreatePinnedToCore(&eq_l_task, "eq_task_l", 10000, NULL, 3, NULL, 1);
	xTaskCreatePinnedToCore(&eq_r_task, "eq_task_r", 10000, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(&write_task, "write_task", 10000, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(&read_task, "read_task", 10000, NULL, 3, NULL, 1);
	ESP_LOGI(MAIN_TAG, "eq task ok");

	ESP_LOGI(MAIN_TAG, "give read_sem, start task chain");
	xSemaphoreGive(read_sem);

	wifi_init_softap();

	ESP_ERROR_CHECK(touch_pad_init());
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	touch_pad_config(TOUCH_PAD_NUM8, 0);

	uint16_t touch_value;
	int http_started = 0;
	httpd_handle_t server;
	while (1) {
		touch_pad_read(TOUCH_PAD_NUM8, &touch_value);
		if(touch_value < 1000) {
			ESP_LOGI(MAIN_TAG, "T:[%d] ", touch_value);
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			touch_pad_read(TOUCH_PAD_NUM8, &touch_value);
			if(touch_value < 1000) {
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
}

