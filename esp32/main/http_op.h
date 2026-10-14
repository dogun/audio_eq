/*
 * http.h
 *
 *  Created on: 2022��5��17��
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

#define HTTP_EDIT_BODY_MAX (CONFIG_SIZE * 3 + 64)
#define HTTP_EQC_BODY_MAX 256

static int _hex_value(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static int url_decode(char* value) {
	char* read = value;
	char* write = value;
	while (*read != '\0') {
		if (*read == '+') {
			*write++ = ' ';
			read++;
		} else if (*read == '%') {
			int high = _hex_value(read[1]);
			int low = read[1] == '\0' ? -1 : _hex_value(read[2]);
			if (high < 0 || low < 0) return 0;
			int decoded = (high << 4) | low;
			if (decoded == 0) return 0;
			*write++ = (char)decoded;
			read += 3;
		} else {
			*write++ = *read++;
		}
	}
	*write = '\0';
	return 1;
}

static int _form_values(char* body,
		const char* first_name, char** first_value,
		const char* second_name, char** second_value) {
	char* cursor = body;
	char* field;
	*first_value = NULL;
	*second_value = NULL;

	while ((field = strsep(&cursor, "&")) != NULL) {
		char* equals = strchr(field, '=');
		if (equals == NULL) continue;
		*equals++ = '\0';
		if (strcmp(field, first_name) == 0) {
			if (!url_decode(equals)) return 0;
			*first_value = equals;
		} else if (strcmp(field, second_name) == 0) {
			if (!url_decode(equals)) return 0;
			*second_value = equals;
		}
	}
	return 1;
}

static esp_err_t _recv_body(httpd_req_t* req, size_t max_size, char** body) {
	if ((size_t)req->content_len > max_size) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "request body too large");
		return ESP_FAIL;
	}

	char* content = malloc((size_t)req->content_len + 1);
	if (content == NULL) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
		return ESP_ERR_NO_MEM;
	}

	size_t received = 0;
	while (received < (size_t)req->content_len) {
		int ret = httpd_req_recv(req, content + received,
				(size_t)req->content_len - received);
		if (ret <= 0) {
			free(content);
			if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
			return ESP_FAIL;
		}
		received += (size_t)ret;
	}
	content[received] = '\0';
	*body = content;
	return ESP_OK;
}

static char* _html_escape(const char* value) {
	size_t capacity = strlen(value) * 6 + 1;
	char* escaped = malloc(capacity);
	if (escaped == NULL) return NULL;

	char* out = escaped;
	while (*value != '\0') {
		const char* entity = NULL;
		switch (*value) {
			case '&': entity = "&amp;"; break;
			case '<': entity = "&lt;"; break;
			case '>': entity = "&gt;"; break;
			case '"': entity = "&quot;"; break;
			case '\'': entity = "&#39;"; break;
			default: *out++ = *value++; continue;
		}
		size_t len = strlen(entity);
		memcpy(out, entity, len);
		out += len;
		value++;
	}
	*out = '\0';
	return escaped;
}

esp_err_t get_handler_edit(httpd_req_t* req) {
	char* separator = strchr(req->uri, '=');
	if (separator == NULL) {
		httpd_resp_send_404(req);
		return ESP_FAIL;
	}

	separator++;
	size_t key_len = strcspn(separator, "&");
	if (key_len == 0 || key_len >= NVS_KEY_NAME_MAX_SIZE) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "config key too long");
		return ESP_FAIL;
	}

	char key[NVS_KEY_NAME_MAX_SIZE] = {0};
	memcpy(key, separator, key_len);
	if (!url_decode(key) || key[0] == '\0') {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid config key");
		return ESP_FAIL;
	}

	char* config = calloc(CONFIG_SIZE, 1);
	if (config == NULL) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
		return ESP_ERR_NO_MEM;
	}
	read_config(key, config, CONFIG_SIZE);
	char* escaped_key = _html_escape(key);
	char* escaped_config = _html_escape(config);
	free(config);
	if (escaped_key == NULL || escaped_config == NULL) {
		free(escaped_key);
		free(escaped_config);
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
		return ESP_ERR_NO_MEM;
	}

	httpd_resp_set_type(req, "text/html; charset=utf-8");
	esp_err_t err = httpd_resp_send_chunk(req,
			"<html><body><form method=\"post\"><input type=\"hidden\" name=\"k\" value=\"",
			HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, escaped_key, HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req,
			"\" /><textarea name=\"eq\" style=\"width:300px;height:600px\">",
			HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, escaped_config, HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req,
			"</textarea><input type=\"submit\" value=\"OK\" /></form></body></html>",
			HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, NULL, 0);
	free(escaped_key);
	free(escaped_config);
	return err;
}

esp_err_t post_handler_edit(httpd_req_t* req) {
	char* content = NULL;
	if (_recv_body(req, HTTP_EDIT_BODY_MAX, &content) != ESP_OK) return ESP_FAIL;

	char* config_key = NULL;
	char* data = NULL;
	if (!_form_values(content, "k", &config_key, "eq", &data)
			|| config_key == NULL || data == NULL) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing config key");
		return ESP_FAIL;
	}
	if (strlen(config_key) == 0 || strlen(config_key) >= NVS_KEY_NAME_MAX_SIZE) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "config key too long");
		return ESP_FAIL;
	}
	if (strlen(data) >= CONFIG_SIZE) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "EQ config too large");
		return ESP_FAIL;
	}
	esp_err_t err = save_config(config_key, data);
	free(content);
	if (err != ESP_OK) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "cannot save config");
		return err;
	}

	char* resp = "OK";
	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t get_handler_eqc(httpd_req_t* req) {
	char config_key_l[64] = {0};
	char config_key_r[64] = {0};
	read_config(CONFIG_KEY_L, config_key_l, sizeof(config_key_l));
	read_config(CONFIG_KEY_R, config_key_r, sizeof(config_key_r));
	char* escaped_l = _html_escape(config_key_l);
	char* escaped_r = _html_escape(config_key_r);
	if (escaped_l == NULL || escaped_r == NULL) {
		free(escaped_l);
		free(escaped_r);
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
		return ESP_ERR_NO_MEM;
	}
	char* html = "<html><body><form method=\"post\">CONFIG_L:<input name=\"l\" value=\"%s\" /><br />CONFIG_R:<input name=\"r\" value=\"%s\" /><input type=\"submit\" value=\"OK\" /></form></body></html>";
	char data[1024] = {0};
	snprintf(data, sizeof(data), html, escaped_l, escaped_r);
	free(escaped_l);
	free(escaped_r);
	httpd_resp_set_type(req, "text/html; charset=utf-8");
	httpd_resp_send(req, data, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t post_handler_eqc(httpd_req_t* req) {
	char* content = NULL;
	if (_recv_body(req, HTTP_EQC_BODY_MAX, &content) != ESP_OK) return ESP_FAIL;

	char* l_key = NULL;
	char* r_key = NULL;
	if (!_form_values(content, "l", &l_key, "r", &r_key)
			|| l_key == NULL || r_key == NULL) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing config key");
		return ESP_FAIL;
	}
	if (strlen(l_key) == 0 || strlen(r_key) == 0
			|| strlen(l_key) >= NVS_KEY_NAME_MAX_SIZE
			|| strlen(r_key) >= NVS_KEY_NAME_MAX_SIZE) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "config key too long");
		return ESP_FAIL;
	}

	esp_err_t left_err = save_config(CONFIG_KEY_L, l_key);
	esp_err_t right_err = save_config(CONFIG_KEY_R, r_key);
	free(content);
	if (left_err != ESP_OK || right_err != ESP_OK) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "cannot save config");
		return ESP_FAIL;
	}

	char* resp = "OK";
	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

httpd_uri_t uri_get_c = {
		.uri = "/eqc",
		.method = HTTP_GET,
		.handler = get_handler_eqc,
		.user_ctx = NULL
};

httpd_uri_t uri_post_c = {
		.uri = "/eqc",
		.method = HTTP_POST,
		.handler = post_handler_eqc,
		.user_ctx = NULL
};

httpd_uri_t uri_get_eq = {
		.uri = "/edit",
		.method = HTTP_GET,
		.handler = get_handler_edit,
		.user_ctx = NULL
};

httpd_uri_t uri_post_eq = {
		.uri = "/edit",
		.method = HTTP_POST,
		.handler = post_handler_edit,
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
