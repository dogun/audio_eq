#ifndef MAIN_AUDIO_SAMPLE_H_
#define MAIN_AUDIO_SAMPLE_H_

#include <math.h>
#include <stdint.h>

#define AUDIO_S24_MIN (-8388608)
#define AUDIO_S24_MAX 8388607

static inline int32_t audio_i2s_to_s24(int32_t raw) {
	uint32_t shifted = (uint32_t)raw << 1;
	return (int32_t)shifted >> 8;
}

static inline int32_t audio_clip_s24(int32_t sample) {
	if (sample < AUDIO_S24_MIN) return AUDIO_S24_MIN;
	if (sample > AUDIO_S24_MAX) return AUDIO_S24_MAX;
	return sample;
}

static inline int32_t audio_clip_float_s24(float sample) {
	if (isnan(sample)) return 0;
	if (sample <= (float)AUDIO_S24_MIN) return AUDIO_S24_MIN;
	if (sample >= (float)AUDIO_S24_MAX) return AUDIO_S24_MAX;
	return (int32_t)sample;
}

static inline int32_t audio_s24_to_i2s(int32_t sample) {
	uint32_t encoded = (uint32_t)audio_clip_s24(sample) & 0x00FFFFFFU;
	return (int32_t)(encoded << 7);
}

#endif /* MAIN_AUDIO_SAMPLE_H_ */
