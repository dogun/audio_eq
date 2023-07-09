/*
 * i2s_config.h
 *
 *  Created on: 2022��5��6��
 *      Author: yuexq
 */

#ifndef MAIN_I2S_CONFIG_H_
#define MAIN_I2S_CONFIG_H_

#include <driver/i2s.h>
#include "common_config.h"

//start i2s config

static const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number

static const i2s_config_t i2s_config = {
		.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
		.sample_rate = RATE,
		.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
		.communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S),
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
		.dma_buf_count = 8,
		.dma_buf_len = 1024,
		.use_apll = true,
		.tx_desc_auto_clear = true,
		//.mclk_multiple=512,
		//.fixed_mclk = 24576000,
		//.fixed_mclk = RATE * 256,
};

static const i2s_pin_config_t pin_config = {
		.mck_io_num = 0,
		//.bck_io_num = 27,
		//.ws_io_num = 26,
		//.data_out_num = 25,
		//.data_in_num = 23
		.bck_io_num = 26,
		.ws_io_num = 27,
		.data_out_num = 33,
		.data_in_num = 25
};
//end i2s config

#endif /* MAIN_I2S_CONFIG_H_ */
