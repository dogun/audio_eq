/*
 * pwmout.h
 *
 *  Created on: 2022Äê6ÔÂ7ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_PWMOUT_H_
#define MAIN_PWMOUT_H_

#include <stdio.h>
#include <math.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/ledc.h>
#include <esp_log.h>

#include "../config/main_config.h"

#define LOG_TAG_PWMOUT "pwmout"

static int max_duty = (int)powf(2, 10) - 1;

typedef struct {
	ledc_timer_config_t timer;
	ledc_channel_config_t channel;
} PWM_CHANNEL;

/**
 * CONFIG pwm_timer bit freq timer
 * CONFIG pwm_timer 10 21000 0
 */
ledc_timer_config_t _get_pwm_timer_config(MAIN_CONFIG* config) {
	MAIN_CONFIG_NODE node = get_config_by_name(config, PWM_TIMER_CONFIG);

	if (node.param_cnt < 3) {
		ESP_LOGE(LOG_TAG_PWMOUT, "pwm_timer config node param error");
		ledc_timer_config_t t = {};
		return t;
	}
	int bit = atoi(node.params[0]);
	int freq = atoi(node.params[1]);
	int timer = atoi(node.params[2]);

	ledc_timer_bit_t b = LEDC_TIMER_1_BIT;
	if (bit == 2) b = LEDC_TIMER_2_BIT;
	else if (bit == 3) b = LEDC_TIMER_3_BIT;
	else if (bit == 4) b = LEDC_TIMER_4_BIT;
	else if (bit == 5) b = LEDC_TIMER_5_BIT;
	else if (bit == 6) b = LEDC_TIMER_6_BIT;
	else if (bit == 7) b = LEDC_TIMER_7_BIT;
	else if (bit == 8) b = LEDC_TIMER_8_BIT;
	else if (bit == 9) b = LEDC_TIMER_9_BIT;
	else if (bit == 10) b = LEDC_TIMER_10_BIT;
	else if (bit == 11) b = LEDC_TIMER_11_BIT;
	else if (bit == 12) b = LEDC_TIMER_12_BIT;
	else if (bit == 13) b = LEDC_TIMER_13_BIT;
	else if (bit == 14) b = LEDC_TIMER_14_BIT;

	ledc_timer_t t = LEDC_TIMER_0;
	if (timer == 1) t = LEDC_TIMER_1;
	else if (timer == 2) t = LEDC_TIMER_2;
	else if (timer == 3) t = LEDC_TIMER_3;

	ledc_timer_config_t pwm_timer = {
		.duty_resolution = b,
		.freq_hz = freq,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_num = t,
		.clk_cfg = LEDC_AUTO_CLK
	};

	return pwm_timer;
}

/**
 * CONFIG pwm_channel channel pin timer
 * CONFIG pwm_channel 18
 */
ledc_channel_config_t _get_pwm_channel_config(MAIN_CONFIG* config) {
	MAIN_CONFIG_NODE node = get_config_by_name(config, PWM_CHANNEL_CONFIG);
	if (node.param_cnt < 3) {
		ESP_LOGE(LOG_TAG_PWMOUT, "pwm_channel config node param error");
		ledc_channel_config_t t = {};
		return t;
	}
	int channel = atoi(node.params[0]);
	int pin = atoi(node.params[1]);
	int timer = atoi(node.params[2]);

	ledc_channel_t ch = LEDC_CHANNEL_0;
	if (channel == 1) ch = LEDC_CHANNEL_1;
	else if (channel == 2) ch = LEDC_CHANNEL_2;
	else if (channel == 3) ch = LEDC_CHANNEL_3;
	else if (channel == 4) ch = LEDC_CHANNEL_4;
	else if (channel == 5) ch = LEDC_CHANNEL_5;

	ledc_timer_t t = LEDC_TIMER_0;
	if (timer == 1) t = LEDC_TIMER_1;
	else if (timer == 2) t = LEDC_TIMER_2;
	else if (timer == 3) t = LEDC_TIMER_3;

	ledc_channel_config_t pwm_channel = {
		.channel    = ch,
		.duty       = 0,
		.gpio_num   = pin,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.hpoint     = 0,
		.timer_sel  = t,
		.flags.output_invert = 0
	};
	return pwm_channel;
}

PWM_CHANNEL init_pwm(MAIN_CONFIG* config) {
	if (NULL == config) {
		ESP_LOGE(LOG_TAG_PWMOUT, "NULL config");
		PWM_CHANNEL c = {};
		return c;
	}

	ledc_timer_config_t pwm_timer = _get_pwm_timer_config(config);
	ledc_channel_config_t pwm_channel = _get_pwm_channel_config(config);

	PWM_CHANNEL ret = {
			.timer = pwm_timer,
			.channel = pwm_channel
	};

	ledc_timer_config(&pwm_timer);
    ledc_channel_config(&pwm_channel);

    return ret;
}

void pwm_set_duty(PWM_CHANNEL channel, float duty) {
	if (duty > 1) duty = 1;
	int real_duty = (int)(duty * (float)max_duty);
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel.channel.channel, real_duty);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel.channel.channel);
}

#endif /* MAIN_PWMOUT_H_ */
