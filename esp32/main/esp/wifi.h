/*
 * wifi.h
 *
 *  Created on: 2022年7月15日
 *      Author: yuexu
 */

#ifndef MAIN_ESP_WIFI_H_
#define MAIN_ESP_WIFI_H_

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>

#include "../config/main_config.h"

#define WIFI_TAG "wifi"

void _wifi_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data) {
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event =
				(wifi_event_ap_staconnected_t*) event_data;
		ESP_LOGI(WIFI_TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac),
				event->aid);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event =
				(wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGI(WIFI_TAG, "station "MACSTR" leave, AID=%d",
				MAC2STR(event->mac), event->aid);
	}
}

/**
 * CONFIG wifi ssid password
 * CONFIG wifi esp32-ap 1234567890
 */
void init_wifi_softap(MAIN_CONFIG* config) {
	char* ssid = "esp32-ap";
	char* password = "1234567890";
	if (NULL != config) { //首次运行时，也要能打开wifi，提供http服务，以便写入config
		MAIN_CONFIG_NODE node = get_config_by_name(config, WIFI_CONFIG);
		if (node.param_cnt < 2) {
			ESP_LOGE(WIFI_TAG, "config node param error: %d", node.param_cnt);
		} else {
			ssid = node.params[0];
			password = node.params[1];
		}
	} else {
		ESP_LOGE(WIFI_TAG, "NULL config");
	}

	wifi_config_t wifi_config = {
			.ap = {
					.authmode = WIFI_AUTH_WPA_WPA2_PSK,
			}
	};
	wifi_config.ap.max_connection = 4;

	bzero(wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid));
	bzero(wifi_config.ap.password, sizeof(wifi_config.ap.password));
	strcpy((char*)wifi_config.ap.ssid, ssid);
	strcpy((char*)wifi_config.ap.password, password);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

	ESP_LOGI(WIFI_TAG, "wifi init ssid:(%s), pass:(%s)", wifi_config.ap.ssid, wifi_config.ap.password);
}

#endif /* MAIN_ESP_WIFI_H_ */
