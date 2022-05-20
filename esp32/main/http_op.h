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
#include "config_store.h"

void url_decode(char* res) {
	ESP_LOGI(HTTP_TAG, "urldecode res: %s", res);
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
	ESP_LOGI(HTTP_TAG, "urldecode res ok: %s", res);
}

esp_err_t get_handler_eq(httpd_req_t* req) {
	char* uri = strstr(req->uri, "=");

	if (NULL == uri) {
		httpd_resp_send_404(req);
		return ESP_FAIL;
	}

	uri += 1;

	char config[700] = {0};
	read_config(uri, config, sizeof(config));
	char* html = "<html><body><form method=\"post\"><input type=\"hidden\" name=\"k\" value=\"%s\" /><textarea name=\"eq\" style=\"width:300px;height:600px\">%s</textarea><input type=\"submit\" value=\"OK\" /></form></body></html>";
	char data[1024] = {0};
	sprintf(data, html, uri, config);
	httpd_resp_send(req, data, strlen(data));
	return ESP_OK;
}

esp_err_t post_handler_eq(httpd_req_t* req) {
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

	char* token;
	char* cc = (char*)content;
	int i = 0;
	char* config_key = NULL;
	char* data = NULL;
	while ((token = strsep(&cc, "=")) != NULL) {
		if (i == 1)
			config_key = token;
		else if (i == 2)
			data = token;
		i++;
	}

	char* sp = strstr(config_key, "&");
	if (NULL != sp) sp[0] = 0;
	save_config(config_key, data);

	char* resp = "OK";
	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t get_handler_c(httpd_req_t* req) {
	char config_key_l[64] = {0};
	char config_key_r[64] = {0};
	read_config(CONFIG_KEY_L, config_key_l, sizeof(config_key_l));
	read_config(CONFIG_KEY_R, config_key_r, sizeof(config_key_r));
	char* html = "<html><body><form method=\"post\">CONFIG_L:<input name=\"l\" value=\"%s\" /><br />CONFIG_R:<input name=\"r\" value=\"%s\" /><input type=\"submit\" value=\"OK\" /></form></body></html>";
	char data[1024] = {0};
	sprintf(data, html, config_key_l, config_key_r);
	httpd_resp_send(req, data, strlen(data));
	return ESP_OK;
}

esp_err_t post_handler_c(httpd_req_t* req) {
	char content[512] = {0};
	int recv_size = MIN(req->content_len, sizeof(content));
	int ret = httpd_req_recv(req, content, recv_size);
	if (ret <= 0) {
		if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
			httpd_resp_send_408(req);
		}
		return ESP_FAIL;
	}

	url_decode(content);
	char* token;
	char* cc = (char*)content;
	int i = 0;
	char* l_key = NULL;
	char* r_key = NULL;
	while ((token = strsep(&cc, "=")) != NULL) {
		ESP_LOGI(HTTP_TAG, "token:%s", token);
		if (i == 1)
			l_key = token;
		else if (i == 2)
			r_key = token;
		i++;
	}
	char* sp = strstr(l_key, "&");
	if (NULL != sp) sp[0] = 0;

	//ESP_LOGI(HTTP_TAG, "config_key l: %s, r:%s", l_key, r_key);
	if (NULL != l_key && NULL != r_key) {
		save_config(CONFIG_KEY_L, l_key);
		save_config(CONFIG_KEY_R, r_key);
	}

	char* resp = "OK";
	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

httpd_uri_t uri_get_c = {
		.uri = "/c",
		.method = HTTP_GET,
		.handler = get_handler_c,
		.user_ctx = NULL
};

httpd_uri_t uri_post_c = {
		.uri = "/c",
		.method = HTTP_POST,
		.handler = post_handler_c,
		.user_ctx = NULL
};

httpd_uri_t uri_get_eq = {
		.uri = "/eq",
		.method = HTTP_GET,
		.handler = get_handler_eq,
		.user_ctx = NULL
};

httpd_uri_t uri_post_eq = {
		.uri = "/eq",
		.method = HTTP_POST,
		.handler = post_handler_eq,
		.user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	httpd_handle_t server = NULL;

	if (httpd_start(&server, &config) == ESP_OK) {
		httpd_register_uri_handler(server, &uri_get_c);
		httpd_register_uri_handler(server, &uri_post_c);
		httpd_register_uri_handler(server, &uri_get_eq);
		httpd_register_uri_handler(server, &uri_post_eq);

	}
	return server;
}

void stop_webserver(httpd_handle_t server) {
	if (server) {
		httpd_stop(server);
	}
}

#endif /* MAIN_HTTP_OP_H_ */
