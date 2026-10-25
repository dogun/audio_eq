#ifndef TEST_ESP_LOG_H_
#define TEST_ESP_LOG_H_

#define ESP_LOG_NONE 0
#define ESP_LOG_ERROR 1
#define ESP_LOG_WARN 2
#define ESP_LOG_INFO 3
#define ESP_LOG_DEBUG 4
#define ESP_LOG_VERBOSE 5

#define ESP_LOGE(tag, fmt, ...) ((void)(tag), (void)(fmt))
#define ESP_LOGW(tag, fmt, ...) ((void)(tag), (void)(fmt))
#define ESP_LOGI(tag, fmt, ...) ((void)(tag), (void)(fmt))
#define ESP_LOGD(tag, fmt, ...) ((void)(tag), (void)(fmt))
#define ESP_LOGV(tag, fmt, ...) ((void)(tag), (void)(fmt))

#endif /* TEST_ESP_LOG_H_ */
