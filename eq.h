#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "snd.h"

#define TWOPI (2.0 * M_PI)
#define PREAMP -5.0f
#define PREAMPF pow(10.0f, PREAMP / 20.0f)
#define MAX_EQ_COUNT 32
#define CONFIG_FILE_R_SUFFIX "_r.conf"
#define CONFIG_FILE_L_SUFFIX "_l.conf"

char eq_file_prefix[128] = {0};
char eq_file_r[128] = {0};
char eq_file_l[128] = {0};

typedef struct t_biquad t_biquad;
struct t_biquad {
	double a0, a1, a2, a3, a4;
	double x1, x2, y1, y2;
	double cf, q, gain;
};

typedef struct eq_config eq_config;
struct eq_config {
	double cf, q, gain;
};

static void process_eq(char* buf, int len, double vol);
static void load_eq_config();

static inline double _apply_biquads_one(double s, t_biquad* b);
static inline void _apply_biquads(int* src, int* dst, int len, double vol);

static void _check_eq_config();
static int _load_config(char* config_file, eq_config* eq, long* config_time, int* eq_len);
static void _build_biquad_list();
static void _mk_biquad(double dbgain, double cf, double q, t_biquad *b);

static eq_config eq_r[MAX_EQ_COUNT];
int eq_len_r;
long config_time_r = 0;

static eq_config eq_l[MAX_EQ_COUNT];
int eq_len_l;
long config_time_l = 0;

static t_biquad r_biquads[MAX_EQ_COUNT];
static t_biquad l_biquads[MAX_EQ_COUNT];

static void load_eq_config(char* eq_file) {
	strcpy(eq_file_prefix, eq_file);
	
	strcpy(eq_file_l, eq_file_prefix);
	strcpy(eq_file_r, eq_file_prefix);
	
	strcpy(eq_file_l + strlen(eq_file_prefix), CONFIG_FILE_L_SUFFIX);
	strcpy(eq_file_r + strlen(eq_file_prefix), CONFIG_FILE_R_SUFFIX);
	
	_check_eq_config();
}

static int delay = 1001;
static void _check_eq_config() {
	if (delay++ < 1000) return;
	delay = 0;

	int res = 0;
	res += _load_config(eq_file_l, eq_l, &config_time_l, &eq_len_l);
	res += _load_config(eq_file_r, eq_r, &config_time_r, &eq_len_r);
	if (res > 0) _build_biquad_list();
}

/*
  fc gain q
 */
static int _load_config(char* config_file, eq_config* eq, long* config_time, int* eq_len) {
	struct stat st;
	int res;
	res = stat(config_file, &st);
	if (res != 0) {
		fprintf(stderr, "stat eq config error:%s, file:%s\n", strerror(res), config_file);
		exit(1);
	}
	long mtime = st.st_mtime;
	if (*config_time >= mtime) { //配置文件未修改
		return 0;
	}
	fprintf(stderr, "reload config:%s\n", config_file);
	
	FILE* fp = fopen(config_file, "r");
	if (fp == NULL) {
		fprintf(stderr, "open eq config error:%s\n", config_file);
		exit(1);
	}
	char line[200];
	(*eq_len) = 0;
	while(!feof(fp)) {
		char* r = fgets(line, sizeof(line), fp);
		if (r == NULL || strlen(r) == 0) continue;
		char* token = strtok(line, " ");
		int i = 0;
		while (token != NULL) {
			//printf("--%s, %d\n", token, i);
			if (i == 0) eq[*eq_len].cf = atof(token);
			else if (i == 1) eq[*eq_len].gain = atof(token);
			else if (i == 2) eq[*eq_len].q = atof(token);
			i++;
			token = strtok(NULL, " ");
		}
		if (i == 3) (*eq_len)++;
	}
	fclose(fp);
	(*config_time) = mtime;
	return 1;
}

static void _build_biquad_list() {
	int i;
	fprintf(stderr, "l eq size:%d, r eq size:%d\n", eq_len_l, eq_len_r);
	for (i = 0; i < eq_len_l; ++i) {
		_mk_biquad(eq_l[i].gain, eq_l[i].cf, eq_l[i].q, &(l_biquads[i]));
		fprintf(stderr, "l: cf %f, gain %f, q %f\n", eq_l[i].cf, eq_l[i].gain, eq_l[i].q);
	}
	
	for (i = 0; i < eq_len_r; ++i) {
		_mk_biquad(eq_r[i].gain, eq_r[i].cf, eq_r[i].q, &(r_biquads[i]));
		fprintf(stderr, "r: cf %f, gain %f, q %f\n", eq_r[i].cf, eq_r[i].gain, eq_r[i].q);
	}
}

/*
 * len为实际char的长度
 */
static void process_eq(char* buf, int len, double vol) {
	int size = len / 4;
	int* ibuf = (int*)buf;
	_check_eq_config();
	_apply_biquads(ibuf, ibuf, size, vol);
}

/* Applies a set of biquadratic filters to a buffer of doubleing point
 * samples.
 * It is safe to have the same input and output buffer.
 * len = 实际按照int的长度
 */
static inline void _apply_biquads(int* src, int* dst, int len, double vol) {
	int i, di;

	if (len % 2 != 0) {
		fprintf(stderr, "buffer len error: %d\n", len);
		exit(1);
	}

	for (i = 0; i < len / 2; ++i) {
		double l_s = (double)*src++ * PREAMPF * vol;
		double r_s = (double)*src++ * PREAMPF * vol;
		//printf("p: %f %f\n", l_s, r_s);
		double l_f = l_s;
		double r_f = r_s;
		
		for (di = 0; di < eq_len_l; ++di) {
			l_f = _apply_biquads_one(l_s, &(l_biquads[di]));
			l_s = l_f;
		}
		for (di = 0; di < eq_len_r; ++di) {
			r_f = _apply_biquads_one(r_s, &(r_biquads[di]));
			r_s = r_f;
		}
		*dst++ = (int)l_f;
		*dst++ = (int)r_f;
		//printf("r: %f %f\n", l_f, r_f);
	}
}

static inline double _apply_biquads_one(double s, t_biquad* b) {
	//printf("----b: %f %f %f addr:%d\n", b->cf,b->q, b->gain, b);
	double f =
		s * b->a0 \
		+ b->a1 * b->x1 \
		+ b->a2 * b->x2 \
		- b->a3 * b->y1 \
		- b->a4 * b->y2;
	b->x2 = b->x1;
	b->x1 = s;
	b->y2 = b->y1;
	b->y1 = f;
	return f;
}


/* biquad functions */

/* Create a Peaking EQ Filter.
 * See 'Audio EQ Cookbook' for more information
 */
static void _mk_biquad(double dbgain, double cf, double q, t_biquad* b) {
  if(b == NULL) {
	  fprintf(stderr, "biquad NULL\n");
	  exit(1);
  }

  double A = pow(10.0, dbgain / 40.0);
  double omega = TWOPI * cf / RATE;
  double sn = sin(omega);
  double cs = cos(omega);
  //double alpha = sn * sinh(M_LN2 / 2.0f * bw * omega / sn);  //use band width
  double alpha = sn / (2 * q);

  double alpha_m_A = alpha * A;
  double alpha_d_A = alpha / A;

  double b0 = 1.0 + alpha_m_A;
  double b1 = -2.0 * cs;
  double b2 = 1.0 - alpha_m_A;
  double a0 = 1.0 + alpha_d_A;
  double a1 = b1;
  double a2 = 1.0 - alpha_d_A;

  b->a0 = b0 / a0;
  b->a1 = b1 / a0;
  b->a2 = b2 / a0;
  b->a3 = a1 / a0;
  b->a4 = a2 / a0;

  b->x1 = 0.0;
  b->x2 = 0.0;
  b->y1 = 0.0;
  b->y2 = 0.0;

  b->cf = cf;
  b->q = q;
  b->gain = dbgain;
}
