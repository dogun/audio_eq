/*
 * i2s_config.h
 *
 *  Created on: 2022��5��6��
 *      Author: yuexq
 */

#ifndef MAIN_I2S_CONFIG_H_
#define MAIN_I2S_CONFIG_H_

#include "driver/i2s_std.h"
#include "hal/i2s_types.h"
#include "common_config.h"

//start i2s config

static i2s_chan_handle_t                tx_chan;
static i2s_chan_handle_t                rx_chan;

static i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                I2S_SLOT_MODE_STEREO),
    .gpio_cfg =
        {
            .mclk = 0,
            .bclk = 27,
            .ws = 26,
            .dout = 25,
            .din = 23,
            .invert_flags =
                {
                    .mclk_inv = false,
                    .bclk_inv = false,
                    .ws_inv = false,
                },
        },
};

		//.bck_io_num = 26,
		//.ws_io_num = 27,
		//.data_out_num = 33,
		//.data_in_num = 25

//end i2s config

static void i2s_init_std_duplex(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

static ssize_t i2s_write(void* buf, size_t len) {
  size_t w_size = 0;
  size_t _len = 0;
  if (len == 0) return 0;
  while (1) {
	  if (i2s_channel_write(tx_chan, buf + _len, len - _len, (size_t *)&w_size, 1000) !=
	      ESP_OK) {
	    return -1;
	  }
	  _len += w_size;
	  if (_len == len) {
		  break;
	  }
  }
  return _len;
}

static ssize_t i2s_read(void* buf, size_t len) {
  size_t r_bytes = 0;
  size_t _len = 0;
  while (1) {
	  if (i2s_channel_read(rx_chan, buf + _len, len - _len, (size_t *)&r_bytes,
	                       1000) != ESP_OK) {
	    return -1;
	  }
	  _len += r_bytes;
	  if (_len == len) break;
  }
  return len;
}

#endif /* MAIN_I2S_CONFIG_H_ */
