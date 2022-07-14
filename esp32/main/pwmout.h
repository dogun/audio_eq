/*
 * pwmout.h
 *
 *  Created on: 2022Äê6ÔÂ7ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_PWMOUT_H_
#define MAIN_PWMOUT_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "math.h"

#define LOG_TAG_PWMOUT "pwmout"

static int max_duty = (int)powf(2, 10) - 1;

ledc_timer_config_t pwm_timer = {
	.duty_resolution = LEDC_TIMER_10_BIT,
	.freq_hz = 21000,
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.timer_num = LEDC_TIMER_0,
	.clk_cfg = LEDC_AUTO_CLK
};

ledc_channel_config_t pwm_channel = {
	.channel    = LEDC_CHANNEL_0,
	.duty       = 0,
	.gpio_num   = 18,
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.hpoint     = 0,
	.timer_sel  = LEDC_TIMER_0,
	.flags.output_invert = 0
};

void config_pwm() {
	ledc_timer_config(&pwm_timer);
    ledc_channel_config(&pwm_channel);
}

void pwm_set_duty(float duty) {
	if (duty > 1) duty = 1;
	int real_duty = (int)(duty * (float)max_duty);
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, real_duty);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

#endif /* MAIN_PWMOUT_H_ */
