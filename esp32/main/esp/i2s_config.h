/*
 * i2s_config.h
 *
 *  Created on: 2022Äê5ÔÂ6ÈÕ
 *      Author: yuexq
 */

#ifndef MAIN_I2S_CONFIG_H_
#define MAIN_I2S_CONFIG_H_

#include <driver/i2s.h>
#include <esp_log.h>
#include "../config/main_config.h"

#define I2S_TAG "i2s"

//start i2s config

/**
 * CONFIG i2s RATE buf_count buf_len mclk_multiple
 * CONFIG i2s 48000 8 1024 256
 */
i2s_config_t _get_i2s_config(MAIN_CONFIG* config) {
	MAIN_CONFIG_NODE node = get_config_by_name(config, I2S_CONFIG);
	if (node.param_cnt < 4) {
		ESP_LOGE(I2S_TAG, "i2s config node param error");
		i2s_config_t c = {};
		return c;
	}
	int rate = atoi(node.params[0]);
	int buf_count = atoi(node.params[1]);
	int buf_len = atoi(node.params[2]);
	int multiple = atoi(node.params[3]);
	i2s_config_t i2s_config = {
			.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
			.sample_rate = rate,
			.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
			.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
			.communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S),
			.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
			.dma_buf_count = buf_count,
			.dma_buf_len = buf_len,
			.use_apll = true,
			.tx_desc_auto_clear = true,
			.mclk_multiple=multiple,
			//.fixed_mclk = 24576000,
			//.fixed_mclk = RATE * 256,
	};
	return i2s_config;
}

/**
 * CONFIG i2s_pin bck lrck out in
 * CONFIG i2s_pin 27 26 25 23
 */
i2s_pin_config_t _get_i2s_pin_config(MAIN_CONFIG* config) {
	MAIN_CONFIG_NODE node = get_config_by_name(config, I2S_PIN_CONFIG);

	if (node.param_cnt < 4) {
		ESP_LOGE(I2S_TAG, "i2s_pin config node param error");
		i2s_pin_config_t n = {};
		return n;
	}

	int bck = atoi(node.params[0]);
	int ws = atoi(node.params[1]);
	int out = atoi(node.params[2]);
	int in = atoi(node.params[3]);

	i2s_pin_config_t pin_config = {
			.mck_io_num = 0,
			.bck_io_num = bck,
			.ws_io_num = ws,
			.data_out_num = out,
			.data_in_num = in
	};
	return pin_config;
}

/**
 * CONFIG i2s_port 0
 */
i2s_port_t _get_i2s_port(MAIN_CONFIG* config) {
	MAIN_CONFIG_NODE node = get_config_by_name(config, I2S_PORT_CONFIG);
	if (node.param_cnt < 1) {
		ESP_LOGE(I2S_TAG, "i2s_port config node param error");
		return -1;
	}

	int num = atoi(node.params[0]);
	i2s_port_t i2s_num = I2S_NUM_0;
	if (num == 1) {
		i2s_num = I2S_NUM_1;
	}

	return i2s_num;
}

void install_i2s(MAIN_CONFIG* config) {
	if (NULL == config) {
		ESP_LOGE(I2S_TAG, "NULL config");
		return;
	}

	i2s_port_t i2s_num = _get_i2s_port(config);

	ESP_LOGI(I2S_TAG, "get i2s config");
	i2s_config_t i2s_config = _get_i2s_config(config);
	ESP_LOGI(I2S_TAG, "get i2s_pin config");
	i2s_pin_config_t pin_config = _get_i2s_pin_config(config);

	ESP_LOGI(I2S_TAG, "init i2s start");
	i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	i2s_set_pin(i2s_num, &pin_config);
	ESP_LOGI(I2S_TAG, "init i2s end");
}

void uninstall_i2s(MAIN_CONFIG* config) {
	if (NULL == config) {
		ESP_LOGE(I2S_TAG, "NULL config");
		return;
	}

	i2s_port_t i2s_num = _get_i2s_port(config);
	i2s_driver_uninstall(i2s_num);
}

//end i2s config

#endif /* MAIN_I2S_CONFIG_H_ */
