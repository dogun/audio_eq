/*
 * common_config.h
 *
 *  Created on: 2022Äê5ÔÂ6ÈÕ
 *      Author: yuexq
 */

#ifndef MAIN_COMMON_CONFIG_H_
#define MAIN_COMMON_CONFIG_H_

#define RATE 48000.0f

#define L_CODE 0
#define R_CODE 1

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

#define READ_TAG "read"
#define WRITE_TAG "write"
#define MAIN_TAG "main"
#define EQ_TAG "eq"
#define HTTP_TAG "http"
#define CONFIG_TAG "config"
#define FILTER_TAG "filter"

#endif /* MAIN_COMMON_CONFIG_H_ */
