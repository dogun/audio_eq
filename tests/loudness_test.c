#include <assert.h>
#include <complex.h>
#include <math.h>
#include <stdio.h>

#include "../esp32/main/eq.h"

static double shelf_response_db(const t_biquad* b, double freq) {
	const double pi = 3.14159265358979323846;
	double w = 2.0 * pi * freq / (double)RATE;
	double complex z1 = cexp(-I * w);
	double complex z2 = z1 * z1;
	double complex num = b->a0 + b->a1 * z1 + b->a2 * z2;
	double complex den = 1.0 + b->a3 * z1 + b->a4 * z2;
	return 20.0 * log10(cabs(num / den));
}

static void fill_level(int32_t* buf, int len, int32_t amplitude) {
	for (int i = 0; i < len; ++i) {
		buf[i] = audio_s24_to_i2s((i & 1) ? -amplitude : amplitude);
	}
}

int main(void) {
	int32_t buf[512];

	loudness_enabled = 1;
	_loudness_reset();
	fill_level(buf, 512, 4194304); /* about -6 dBFS */
	loudness_update(buf, 512);
	float loud_factor = loudness_factor;
	assert(loud_factor < 0.01f);

	_loudness_reset();
	fill_level(buf, 512, 8389); /* about -60 dBFS */
	for (int i = 0; i < 10; ++i) loudness_update(buf, 512);
	float quiet_factor = loudness_factor;
	assert(quiet_factor > 0.5f);
	loudness_enabled = 0;

	printf("level mapping: loud factor %.3f, quiet factor %.3f\n",
			loud_factor, quiet_factor);

	loudness_ref_db = -30.0f;
	loudness_range_db = 20.0f;
	loudness_gate_db = -80.0f;
	loudness_enabled = 1;
	_loudness_reset();
	fill_level(buf, 512, 838861); /* about -20 dBFS, above new ref */
	loudness_update(buf, 512);
	float above_ref = loudness_factor;
	assert(above_ref < 0.01f);

	_loudness_reset();
	fill_level(buf, 512, 47180); /* about -45 dBFS */
	for (int i = 0; i < 10; ++i) loudness_update(buf, 512);
	float below_ref = loudness_factor;
	assert(below_ref > 0.4f);

	loudness_ref_db = LOUDNESS_DEFAULT_REF_DBFS;
	loudness_range_db = LOUDNESS_DEFAULT_RANGE_DB;
	loudness_gate_db = LOUDNESS_DEFAULT_GATE_DBFS;
	loudness_enabled = 0;

	printf("threshold mapping: above-ref %.3f, below-ref %.3f\n",
			above_ref, below_ref);

	t_biquad low = {0};
	t_biquad high = {0};
	t_biquad flat_low = {0};
	t_biquad flat_high = {0};

	_mk_shelf(LOUDNESS_DEFAULT_BASS_DB, LOUDNESS_BASS_HZ,
			LOUDNESS_SHELF_Q, 0, &low);
	_mk_shelf(LOUDNESS_DEFAULT_TREBLE_DB, LOUDNESS_TREBLE_HZ,
			LOUDNESS_SHELF_Q, 1, &high);
	_mk_shelf(0.0f, LOUDNESS_BASS_HZ, LOUDNESS_SHELF_Q, 0, &flat_low);
	_mk_shelf(0.0f, LOUDNESS_TREBLE_HZ, LOUDNESS_SHELF_Q, 1, &flat_high);

	double low_dc = shelf_response_db(&low, 0.1);
	double low_high = shelf_response_db(&low, 10000.0);
	double high_high = shelf_response_db(&high, 20000.0);
	double high_low = shelf_response_db(&high, 100.0);
	double flat_l = shelf_response_db(&flat_low, 1000.0);
	double flat_h = shelf_response_db(&flat_high, 1000.0);

	printf("low shelf:  %.3f dB @0.1Hz, %.3f dB @10kHz\n", low_dc, low_high);
	printf("high shelf: %.3f dB @20kHz, %.3f dB @100Hz\n", high_high, high_low);
	printf("0 dB shelves: %.3f / %.3f dB @1kHz\n", flat_l, flat_h);

	assert(fabs(low_dc - 8.0) < 0.15);
	assert(fabs(low_high) < 0.3);
	assert(fabs(high_high - 4.0) < 0.4);
	assert(fabs(high_low) < 0.4);
	assert(fabs(flat_l) < 0.01);
	assert(fabs(flat_h) < 0.01);

	puts("loudness_test: OK");
	return 0;
}
