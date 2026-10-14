#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "../esp32/main/audio_delay.h"
#include "../esp32/main/audio_sample.h"
#include "../esp32/main/stereo2mono.h"

static void test_round_trip(int32_t sample) {
	int32_t encoded = audio_s24_to_i2s(sample);
	assert(((uint32_t)encoded & 0x7FU) == 0);
	assert(audio_i2s_to_s24(encoded) == sample);
}

int main(void) {
	const int32_t samples[] = {
		AUDIO_S24_MIN, -1000000, -1, 0, 1, 1000000, AUDIO_S24_MAX
	};

	for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
		test_round_trip(samples[i]);
	}

	assert(audio_clip_s24(INT32_MIN) == AUDIO_S24_MIN);
	assert(audio_clip_s24(INT32_MAX) == AUDIO_S24_MAX);
	assert(audio_clip_float_s24(NAN) == 0);
	assert(audio_clip_float_s24(INFINITY) == AUDIO_S24_MAX);
	assert(audio_clip_float_s24(-INFINITY) == AUDIO_S24_MIN);

	int32_t stereo[] = {
		audio_s24_to_i2s(-1), audio_s24_to_i2s(-1),
		audio_s24_to_i2s(AUDIO_S24_MIN), audio_s24_to_i2s(AUDIO_S24_MAX)
	};
	stereo2mono(stereo, stereo, 4);
	assert(audio_i2s_to_s24(stereo[0]) == -1);
	assert(audio_i2s_to_s24(stereo[1]) == -1);
	assert(audio_i2s_to_s24(stereo[2]) == 0);
	assert(audio_i2s_to_s24(stereo[3]) == 0);

	int left_frames = -1;
	int right_frames = -1;
	audio_resolve_relative_delays(-1.0f, 0.0f, 48000.0f, 256,
			&left_frames, &right_frames);
	assert(left_frames == 0);
	assert(right_frames == 48);

	for (int frame = 0; frame < 256; ++frame) {
		int left_index = frame - left_frames;
		int right_index = frame - right_frames;
		int left_sample = left_index >= 0 ? left_index * 2 : 512 + left_index * 2;
		int right_sample = right_index >= 0 ? right_index * 2 + 1
				: 512 + right_index * 2 + 1;
		assert(left_sample >= 0 && left_sample < 512);
		assert(right_sample >= 0 && right_sample < 512);
	}

	audio_resolve_relative_delays(-3.0f, -1.0f, 48000.0f, 256,
			&left_frames, &right_frames);
	assert(left_frames == 0);
	assert(right_frames == 96);

	audio_resolve_relative_delays(1000.0f, 0.0f, 48000.0f, 256,
			&left_frames, &right_frames);
	assert(left_frames == 256);
	assert(right_frames == 0);

	audio_resolve_relative_delays(NAN, 1.0f, 48000.0f, 256,
			&left_frames, &right_frames);
	assert(left_frames == 0);
	assert(right_frames == 48);

	puts("dsp_boundary_test: OK");
	return 0;
}
