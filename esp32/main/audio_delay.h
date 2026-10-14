#ifndef MAIN_AUDIO_DELAY_H_
#define MAIN_AUDIO_DELAY_H_

#include <math.h>

static inline int audio_delay_ms_to_frames(float delay_ms, float rate, int max_frames) {
	if (!isfinite(delay_ms) || delay_ms <= 0.0f || rate <= 0.0f || max_frames <= 0) {
		return isinf(delay_ms) && delay_ms > 0.0f ? max_frames : 0;
	}

	float frames = delay_ms * rate / 1000.0f;
	if (!isfinite(frames) || frames >= (float)max_frames) return max_frames;
	return (int)frames;
}

static inline void audio_resolve_relative_delays(float left_ms, float right_ms,
		float rate, int max_frames, int* left_frames, int* right_frames) {
	if (isnan(left_ms)) left_ms = 0.0f;
	if (isnan(right_ms)) right_ms = 0.0f;

	if (left_ms > right_ms) {
		*left_frames = audio_delay_ms_to_frames(left_ms - right_ms, rate, max_frames);
		*right_frames = 0;
	} else if (right_ms > left_ms) {
		*left_frames = 0;
		*right_frames = audio_delay_ms_to_frames(right_ms - left_ms, rate, max_frames);
	} else {
		*left_frames = 0;
		*right_frames = 0;
	}
}

#endif /* MAIN_AUDIO_DELAY_H_ */
