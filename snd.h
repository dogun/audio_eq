#define ALSA_PCM_NEW_HW_PARAMS_API
 
#include <alsa/asoundlib.h>

#define RATE 48000.0f
#define FRAME_SIZE 192
#define SAMPLE_SIZE 32
#define CHANNEL 2
#define FRAME_BUFFER_X (SAMPLE_SIZE / 8 * CHANNEL)

snd_pcm_uframes_t c_snd_pcm(snd_pcm_t ** handle, char *hw, int record) {
	snd_pcm_hw_params_t * params;
	
	unsigned int val = RATE;
	int dir = 0;
	snd_pcm_uframes_t frames = FRAME_SIZE;
	int ret;
	
	/* open PCM device for recording (capture). */
	// 访问硬件，并判断硬件是否访问成功 
	if (record == 0) ret = snd_pcm_open(handle, hw, SND_PCM_STREAM_PLAYBACK, 0);
	else ret = snd_pcm_open(handle, hw, SND_PCM_STREAM_CAPTURE, 0);
	if( ret < 0 ) {
		fprintf(stderr,
			  "unable to open pcm device: %s, record: %d\n",
			  snd_strerror(ret), record);
		exit(1);
	}

	//播放设备参数
	snd_pcm_hw_params_alloca(&params); //分配params结构体
	snd_pcm_hw_params_any(*handle, params);//初始化params
	snd_pcm_hw_params_set_access(*handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); //初始化访问权限
	snd_pcm_hw_params_set_format(*handle, params, SND_PCM_FORMAT_S32_LE);
	snd_pcm_hw_params_set_channels(*handle, params, 2);
	snd_pcm_hw_params_set_rate_near(*handle, params, &val, &dir);
	//if(record == 0) snd_pcm_hw_params_set_period_size_near(*handle,params,&frames,&dir);
	snd_pcm_hw_params_set_period_size_near(*handle, params, &frames, &dir);
	ret = snd_pcm_hw_params(*handle, params);
	if(ret < 0) {
		fprintf(stderr, "snd_pcm_hw_params error: %s record: %d\n", snd_strerror(ret), record);
		exit(1);
	}
	
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);

	return frames;
}
