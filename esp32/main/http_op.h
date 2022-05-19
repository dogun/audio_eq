/*
 * http.h
 *
 *  Created on: 2022Äê5ÔÂ17ÈÕ
 *      Author: yuexq
 */

#ifndef MAIN_HTTP_OP_H_
#define MAIN_HTTP_OP_H_

#include <ctype.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <esp_http_server.h>

void url_decode(char* res) {
	ESP_LOGI(MAIN_TAG, "urldecode res: %s", res);
	int d = 0; /* whether or not the string is decoded */

	char e_str[] = "00"; /* for a hex code */

	while (!d) {
		d = 1;
		int i; /* the counter for the string */

		for (i = 0; i < strlen(res); ++i) {
			if (res[i] == '%') {
				if (res[i + 1] == 0)
					return;
				if (isxdigit(res[i + 1]) && isxdigit(res[i + 2])) {
					d = 0;
					e_str[0] = res[i + 1];
					e_str[1] = res[i + 2];
					long int x = strtol(e_str, NULL, 16);
					memmove(&res[i + 1], &res[i + 3], strlen(&res[i + 3]) + 1);
					res[i] = x;
				}
			} else if (res[i] == '+') {
				res[i] = ' ';
			}
		}
	}
	ESP_LOGI(MAIN_TAG, "urldecode res ok: %s", res);
}

esp_err_t _get_handler_x(httpd_req_t* req, int ch) {
	char config[700] = {0};
	read_eq(ch, config, 700);
	char* html = "<html><body><form method=\"post\"><textarea name=\"eq\" style=\"width:300px;height:600px\">%s</textarea><input type=\"submit\" value=\"OK\" /></form></body></html>";
	char data[1024] = {0};
	sprintf(data, html, config);
	httpd_resp_send(req, data, strlen(data));
	return ESP_OK;
}

esp_err_t get_handler_l(httpd_req_t* req) {
	return _get_handler_x(req, L_CODE);
}

esp_err_t get_handler_r(httpd_req_t* req) {
	return _get_handler_x(req, R_CODE);
}

esp_err_t _post_handler_x(httpd_req_t* req, int ch) {
	char content[1024] = {0};
	int recv_size = MIN(req->content_len, sizeof(content));
	int ret = httpd_req_recv(req, content, recv_size);
	if (ret <= 0) {
		if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
			httpd_resp_send_408(req);
		}
		return ESP_FAIL;
	}

	url_decode(content);
	save_eq(ch, content + 3);

	char* resp = "OK";
	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t post_handler_l(httpd_req_t* req) {
	return _post_handler_x(req, L_CODE);
}

esp_err_t post_handler_r(httpd_req_t* req) {
	return _post_handler_x(req, R_CODE);
}

httpd_uri_t uri_get_l = {
		.uri = "/l",
		.method = HTTP_GET,
		.handler = get_handler_l,
		.user_ctx = NULL
};

httpd_uri_t uri_post_l = {
		.uri = "/l",
		.method = HTTP_POST,
		.handler = post_handler_l,
		.user_ctx = NULL
};

httpd_uri_t uri_get_r = {
		.uri = "/r",
		.method = HTTP_GET,
		.handler = get_handler_r,
		.user_ctx = NULL
};

httpd_uri_t uri_post_r = {
		.uri = "/r",
		.method = HTTP_POST,
		.handler = post_handler_r,
		.user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	httpd_handle_t server = NULL;

	if (httpd_start(&server, &config) == ESP_OK) {
		httpd_register_uri_handler(server, &uri_get_l);
		httpd_register_uri_handler(server, &uri_post_l);
		httpd_register_uri_handler(server, &uri_get_r);
		httpd_register_uri_handler(server, &uri_post_r);
	}
	return server;
}

void stop_webserver(httpd_handle_t server) {
	if (server) {
		httpd_stop(server);
	}
}

#endif /* MAIN_HTTP_OP_H_ */
