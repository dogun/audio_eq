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

#define HTTP_EDIT_BODY_MAX (CONFIG_SIZE * 3 + 256)

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

typedef struct {
	const char* name;
	char* value;
} form_field_t;

static int _form_parse(char* body, form_field_t* fields, size_t count) {
	char* cursor = body;
	char* field;
	for (size_t i = 0; i < count; ++i) fields[i].value = NULL;

	while ((field = strsep(&cursor, "&")) != NULL) {
		char* equals = strchr(field, '=');
		if (equals == NULL) continue;
		*equals++ = '\0';
		for (size_t i = 0; i < count; ++i) {
			if (strcmp(field, fields[i].name) == 0) {
				if (!url_decode(equals)) return 0;
				fields[i].value = equals;
				break;
			}
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

static char* _eq_key(int ch) {
	return ch == L_CODE ? EQ_KEY_L : EQ_KEY_R;
}

static esp_err_t _get_eq_handler(httpd_req_t* req, int ch) {
	char* key = _eq_key(ch);
	int swap = channel_swap;
	int loud = loudness_enabled;
	float max_bass = loudness_bass_max_db;
	float max_treble = loudness_treble_max_db;
	float ref_db = loudness_ref_db;
	float range_db = loudness_range_db;
	float gate_db = loudness_gate_db;

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

	const char* title = ch == L_CODE ? "Left EQ" : "Right EQ";
	httpd_resp_set_type(req, "text/html; charset=utf-8");
	esp_err_t err = httpd_resp_send_chunk(req, "<html><body><h3>",
			HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, title, HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, " (", HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, escaped_key, HTTPD_RESP_USE_STRLEN);
	char controls[1280];
	snprintf(controls, sizeof(controls),
			"</h3><form method=\"post\">"
			"<p><label><input type=\"checkbox\" name=\"swap\" value=\"1\"%s /> Swap L/R</label></p>"
			"<p><label><input type=\"checkbox\" name=\"loud\" value=\"1\"%s /> Loudness</label></p>"
			"<p>Max bass <input type=\"number\" name=\"bass\" min=\"0\" max=\"12\" step=\"0.5\" value=\"%.1f\" /> dB</p>"
			"<p>Max treble <input type=\"number\" name=\"treble\" min=\"0\" max=\"12\" step=\"0.5\" value=\"%.1f\" /> dB</p>"
			"<p>Ref level <input type=\"number\" name=\"ref\" min=\"-60\" max=\"-6\" step=\"1\" value=\"%.1f\" /> dBFS</p>"
			"<p>Range <input type=\"number\" name=\"range\" min=\"10\" max=\"60\" step=\"1\" value=\"%.1f\" /> dB</p>"
			"<p>Gate <input type=\"number\" name=\"gate\" min=\"-90\" max=\"-40\" step=\"1\" value=\"%.1f\" /> dBFS</p>"
			"<textarea name=\"eq\" style=\"width:300px;height:600px\">",
			swap ? " checked" : "",
			loud ? " checked" : "",
			max_bass, max_treble, ref_db, range_db, gate_db);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, controls, HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, escaped_config, HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req,
			"</textarea><input type=\"submit\" value=\"OK\" /></form></body></html>",
			HTTPD_RESP_USE_STRLEN);
	if (err == ESP_OK) err = httpd_resp_send_chunk(req, NULL, 0);
	free(escaped_key);
	free(escaped_config);
	return err;
}

static esp_err_t _post_eq_handler(httpd_req_t* req, int ch) {
	char* key = _eq_key(ch);

	char* content = NULL;
	if (_recv_body(req, HTTP_EDIT_BODY_MAX, &content) != ESP_OK) return ESP_FAIL;

	form_field_t fields[] = {
		{"eq", NULL}, {"swap", NULL}, {"loud", NULL},
		{"bass", NULL}, {"treble", NULL},
		{"ref", NULL}, {"range", NULL}, {"gate", NULL}
	};
	if (!_form_parse(content, fields, sizeof(fields) / sizeof(fields[0]))
			|| fields[0].value == NULL) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing EQ config");
		return ESP_FAIL;
	}
	char* data = fields[0].value;
	int swap = fields[1].value != NULL && atoi(fields[1].value) != 0;
	int loud = fields[2].value != NULL && atoi(fields[2].value) != 0;
	float bass = fields[3].value != NULL ? atof(fields[3].value) : loudness_bass_max_db;
	float treble = fields[4].value != NULL ? atof(fields[4].value) : loudness_treble_max_db;
	float ref_db = fields[5].value != NULL ? atof(fields[5].value) : loudness_ref_db;
	float range_db = fields[6].value != NULL ? atof(fields[6].value) : loudness_range_db;
	float gate_db = fields[7].value != NULL ? atof(fields[7].value) : loudness_gate_db;
	if (!isfinite(bass) || bass < 0.0f) bass = 0.0f;
	if (bass > 12.0f) bass = 12.0f;
	if (!isfinite(treble) || treble < 0.0f) treble = 0.0f;
	if (treble > 12.0f) treble = 12.0f;
	if (!isfinite(ref_db) || ref_db < -60.0f) ref_db = -60.0f;
	if (ref_db > -6.0f) ref_db = -6.0f;
	if (!isfinite(range_db) || range_db < 10.0f) range_db = 10.0f;
	if (range_db > 60.0f) range_db = 60.0f;
	if (!isfinite(gate_db) || gate_db < -90.0f) gate_db = -90.0f;
	if (gate_db > -40.0f) gate_db = -40.0f;
	if (gate_db >= ref_db) gate_db = ref_db - 1.0f;
	if (strlen(data) >= CONFIG_SIZE) {
		free(content);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "EQ config too large");
		return ESP_FAIL;
	}

	char loud_cfg[64];
	snprintf(loud_cfg, sizeof(loud_cfg), "%d,%.1f,%.1f,%.1f,%.1f,%.1f",
			loud, bass, treble, ref_db, range_db, gate_db);
	esp_err_t err = save_config(key, data);
	if (err == ESP_OK) err = save_config(SWAP_KEY, swap ? "1" : "0");
	if (err == ESP_OK) err = save_config(LOUD_KEY, loud_cfg);
	free(content);
	if (err != ESP_OK) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "cannot save config");
		return err;
	}
	eq_reload_requested = 1;

	httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t get_handler_eq_l(httpd_req_t* req) {
	return _get_eq_handler(req, L_CODE);
}

esp_err_t post_handler_eq_l(httpd_req_t* req) {
	return _post_eq_handler(req, L_CODE);
}

esp_err_t get_handler_eq_r(httpd_req_t* req) {
	return _get_eq_handler(req, R_CODE);
}

esp_err_t post_handler_eq_r(httpd_req_t* req) {
	return _post_eq_handler(req, R_CODE);
}

httpd_uri_t uri_get_eq_l = {
		.uri = "/eq_l",
		.method = HTTP_GET,
		.handler = get_handler_eq_l,
		.user_ctx = NULL
};

httpd_uri_t uri_post_eq_l = {
		.uri = "/eq_l",
		.method = HTTP_POST,
		.handler = post_handler_eq_l,
		.user_ctx = NULL
};

httpd_uri_t uri_get_eq_r = {
		.uri = "/eq_r",
		.method = HTTP_GET,
		.handler = get_handler_eq_r,
		.user_ctx = NULL
};

httpd_uri_t uri_post_eq_r = {
		.uri = "/eq_r",
		.method = HTTP_POST,
		.handler = post_handler_eq_r,
		.user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	httpd_handle_t server = NULL;

	if (httpd_start(&server, &config) == ESP_OK) {
		httpd_register_uri_handler(server, &uri_get_eq_l);
		httpd_register_uri_handler(server, &uri_post_eq_l);
		httpd_register_uri_handler(server, &uri_get_eq_r);
		httpd_register_uri_handler(server, &uri_post_eq_r);

	}
	return server;
}

void stop_webserver(httpd_handle_t server) {
	if (server) {
		httpd_stop(server);
	}
}

#endif /* MAIN_HTTP_OP_H_ */
