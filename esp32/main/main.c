#include "driver/gpio.h"
#include "eq.h"
#include "config_store.h"
#include "eq_config.h"
#include "http_op.h"
#include "i2s_config.h"
#include "maxvol_pwmout.h"
#include "pwmout.h"
#include "stereo2mono.h"


typedef unsigned int size_t;

#define BUF_SIZE 512
static int32_t data1[BUF_SIZE] = {0};
static int32_t data2[BUF_SIZE] = {0};
static int buf_index = 1;
static int stereo2mono_config = 0;
static int maxvol_config = 0;

static SemaphoreHandle_t read_sem;
static SemaphoreHandle_t write_l_sem;
static SemaphoreHandle_t write_r_sem;
static SemaphoreHandle_t eq_l_sem;
static SemaphoreHandle_t eq_r_sem;


#ifdef TEST_1
	int test_count = 0;
	int c0 = 0;
	int c1 = 0;
	int c2 = 0;
	int c3 = 0;
	int c4 = 0;
	int c5 = 0;
	int c6 = 0;
	int c7 = 0;
	int c8 = 0;
	int c9 = 0;
	int p0 = 0;
	int M = 8388608;
	int p = 1;
#endif

void pre_task(int32_t* src, int32_t* dst, int len) {
	if (stereo2mono_config == 1) stereo2mono(src, dst, len);
#ifdef TEST_1
	//For Test
	int i;
	for (i = 0; i < len; i += 2) {
		int32_t rl = src[i];
		int32_t rr = src[i+1];

		rl = (rl << 1) >> 8;
		rr = (rr << 1) >> 8;

		if (test_count >= RATE) {
			ESP_LOGI("TEST", "c0:%d c1:%d c2:%d c3:%d c4:%d c5:%d c6:%d c7:%d c8:%d c9:%d p0:%d", c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, p0);
			test_count = 0;
			c0 = c1 = c2 = c3 = c4 = c5 = c6 = c7 = c8 = c9 = p0 = 0;
		}

		test_count += 1;
		rl = abs(rr) + abs(rl);

		if ((p > 0 && rl / p > 10) || (rl > 0 && p / rl > 10)) p0 += 1;
		p = rl;

		if (rl < M / 10000) c0 += 1;
		else if (rl < M / 10000 * 2) c1 += 1;
		else if (rl < M / 10000 * 3) c2 += 1;
		else if (rl < M / 10000 * 4) c3 += 1;
		else if (rl < M / 10000 * 5) c4 += 1;
		else if (rl < M / 10000 * 6) c5 += 1;
		else if (rl < M / 10000 * 7) c6 += 1;
		else if (rl < M / 10000 * 8) c7 += 1;
		else if (rl < M / 10000 * 9) c8 += 1;
		else c9 += 1;
	}
#endif
}

int32_t* get_buf() {
	if (buf_index == 1) return data1;
	else return data2;
}

int32_t* get_sec_buf() {
	if (buf_index == 1) return data2;
	else return data1;
}

void switch_buf() {
	if (buf_index == 1) buf_index = 2;
	else buf_index = 1;
}

void post_task(int32_t* src, int32_t* dst, int len) {
	if (maxvol_config == 1) maxvol(src, dst, len, -1);
}

void read_task() {
	while (true) {
		xSemaphoreTake(read_sem, portMAX_DELAY);
		int32_t* bf = get_buf();
		i2s_read(bf, BUF_SIZE * sizeof(int32_t));
		pre_task(bf, bf, BUF_SIZE);
		xSemaphoreGive(eq_r_sem);
		xSemaphoreGive(eq_l_sem);
	}
}

void write_task() {
	//换算delay帧数
	int l_f = l_delay * RATE / 1000;
	int r_f = r_delay * RATE / 1000;
	if (l_f > BUF_SIZE / 2) l_f = BUF_SIZE / 2;
	if (r_f > BUF_SIZE / 2) r_f = BUF_SIZE / 2;
	if (l_f > 0 && r_f > 0) { //取差值
		if (l_f > r_f) {
			l_f = l_f - r_f;
			r_f = 0;
		}else {
			r_f = r_f - l_f;
			l_f = 0;
		}
	}

	while (true) {
		xSemaphoreTake(write_l_sem, portMAX_DELAY);
		xSemaphoreTake(write_r_sem, portMAX_DELAY);

		int32_t* bf = get_buf();
		post_task(bf, bf, BUF_SIZE);

		if (l_delay == 0 && r_delay == 0) {
			i2s_write(bf, BUF_SIZE * sizeof(int32_t));
		}else {
			//处理delay
			int32_t data[BUF_SIZE] = {0};
			//先把需要delay的声道的剩余数据，从上一个buf中取出来，再续现在的buf
			int32_t* sec_bf = get_sec_buf();
			for (int i = 0; i < BUF_SIZE / 2; ++i) {
				int l_index = i - l_f;
				int r_index = i - r_f;
				int32_t l_val = (l_index >= 0) ? bf[l_index * 2] : sec_bf[BUF_SIZE + l_index * 2];
				int32_t r_val = (r_index >= 0) ? bf[r_index * 2 + 1] : sec_bf[BUF_SIZE + r_index * 2 + 1];
				data[i * 2] = l_val;
				data[i * 2 + 1] = r_val;
			}
			i2s_write(data, BUF_SIZE * sizeof(int32_t));
		}

		switch_buf();
		xSemaphoreGive(read_sem);
	}
}

void eq_l_task() {
	while (true) {
		xSemaphoreTake(eq_l_sem, portMAX_DELAY);
		int32_t* bf = get_buf();
		_apply_biquads_l(bf, bf, BUF_SIZE);
		xSemaphoreGive(write_l_sem);
	}
}

void eq_r_task() {
	while (true) {
		xSemaphoreTake(eq_r_sem, portMAX_DELAY);
		int32_t* bf = get_buf();
		_apply_biquads_r(bf, bf, BUF_SIZE);
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
		ESP_LOGI(MAIN_TAG, "station %s join, AID=%d", (char*)event->mac,
				event->aid);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event =
				(wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGI(MAIN_TAG, "station %s leave, AID=%d",
				(char*)event->mac, event->aid);
	}
}


esp_netif_t* wifi_ap;
void wifi_init_softap(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_ap = esp_netif_create_default_wifi_ap();

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

void wifi_deinit_softap(void) {
	esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
	ESP_ERROR_CHECK(esp_wifi_deinit());
	esp_netif_destroy_default_wifi(wifi_ap);
	ESP_ERROR_CHECK(esp_event_loop_delete_default());
	esp_netif_deinit();
}

#define WIFI_ON GPIO_NUM_33


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

	ESP_LOGI(MAIN_TAG, "read maxvol config");
	char maxvol[16] = {0};
	read_config("maxvol", maxvol, sizeof(maxvol));
	if (strlen(maxvol) > 0) maxvol_config = atoi(maxvol);
	if (maxvol_config == 1) {
		config_pwm();
	}
	ESP_LOGI(MAIN_TAG, "read maxvol config: %s", maxvol);

	ESP_LOGI(MAIN_TAG, "init i2s start");
	i2s_init_std_duplex();
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

	gpio_reset_pin(WIFI_ON);
	gpio_set_direction(WIFI_ON, GPIO_MODE_INPUT);
	gpio_reset_pin(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_reset_pin(GPIO_NUM_16);
	gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
	gpio_reset_pin(GPIO_NUM_17);
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
	gpio_reset_pin(GPIO_NUM_18);
	gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
	
	gpio_set_level(GPIO_NUM_16, 1);
	gpio_set_level(GPIO_NUM_17, 0);
	gpio_set_level(GPIO_NUM_18, 0);

	int http_started = 0;
	int amp_enabled = 0;
	httpd_handle_t server;
	while (1) {
		int gpio_level = gpio_get_level(WIFI_ON);
		//ESP_LOGI(MAIN_TAG, "T:[%d] ", gpio_level);

		if(gpio_level == 1) {
			ESP_LOGI(MAIN_TAG, "T:[%d] ", gpio_level);
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			gpio_level = gpio_get_level(WIFI_ON);
			if(gpio_level == 1) {
				if (http_started == 0) {
					wifi_init_softap();
					ESP_LOGI(MAIN_TAG, "start http server");
					ESP_ERROR_CHECK(esp_wifi_start());
					server = start_webserver();
					gpio_set_level(GPIO_NUM_2, 1);
					http_started = 1;
				}else {
					stop_webserver(server);
					esp_wifi_stop();
					wifi_deinit_softap();
					http_started = 0;
					gpio_set_level(GPIO_NUM_2, 0);
				}
				vTaskDelay(3000 / portTICK_PERIOD_MS);
			}
		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
		if (amp_enabled == 0) {
			gpio_set_level(GPIO_NUM_18, 1);
			gpio_set_level(GPIO_NUM_17, 1);
			gpio_set_level(GPIO_NUM_16, 0);
			ESP_LOGI(MAIN_TAG, "set 16 17 18 high");
			amp_enabled = 1;
		}
	}
}

