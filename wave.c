#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int32_t  sint32;

// WAV 文件头
typedef struct WaveHeader {
	uint8  riff[4];             // 资源交换文件标志
	uint32 size;                // 从下个地址开始到文件结尾的字节数
	uint8  wave_flag[4];        // wave 文件标识
	uint8  fmt[4];              // 波形格式标识
	uint32 fmt_len;             // fmt 块长度
	uint16 tag;                 // 格式种类，值为 1 时表示 PCM 线性编码
	uint16 channels;            // 通道数
	uint32 samp_freq;           // 采样频率
	uint32 byte_rate;           // 每秒字节数
	uint16 block_align;         // 块对齐字节数
	uint16 bit_samp;            // 每采样位数
} wave_header_t;

typedef struct WaveStruct {
	FILE* fp;                   // 文件指针
	wave_header_t header;       // 文件头
	uint8 data_flag[4];         // 数据标识符
	uint32 length;              // 数据长度
} wave_t;

static wave_t wave;

static int _read_exact(void* dst, size_t size, FILE* fp) {
	return fread(dst, 1, size, fp) == size;
}

static int _wave_error(const char* what) {
	fprintf(stderr, "wave: %s\n", what);
	if (wave.fp != NULL) {
		fclose(wave.fp);
		wave.fp = NULL;
	}
	return 0;
}

/*
 * 打开并解析 *.wav 文件。成功返回 1，失败返回 0。
 */
int WaveOpen(char* file) {
	memset(&wave, 0, sizeof(wave));

	wave.fp = fopen(file, "rb");
	if (wave.fp == NULL) {
		fprintf(stderr, "wave: cannot open %s\n", file);
		return 0;
	}

	if (!_read_exact(wave.header.riff, 4, wave.fp)) return _wave_error("truncated RIFF header");
	if (!_read_exact(&wave.header.size, 4, wave.fp)) return _wave_error("truncated RIFF size");
	if (!_read_exact(wave.header.wave_flag, 4, wave.fp)) return _wave_error("truncated WAVE header");
	if (!_read_exact(wave.header.fmt, 4, wave.fp)) return _wave_error("truncated fmt header");
	if (!_read_exact(&wave.header.fmt_len, 4, wave.fp)) return _wave_error("truncated fmt length");
	if (!_read_exact(&wave.header.tag, 2, wave.fp)) return _wave_error("truncated fmt tag");
	if (!_read_exact(&wave.header.channels, 2, wave.fp)) return _wave_error("truncated channels");
	if (!_read_exact(&wave.header.samp_freq, 4, wave.fp)) return _wave_error("truncated sample rate");
	if (!_read_exact(&wave.header.byte_rate, 4, wave.fp)) return _wave_error("truncated byte rate");
	if (!_read_exact(&wave.header.block_align, 2, wave.fp)) return _wave_error("truncated block align");
	if (!_read_exact(&wave.header.bit_samp, 2, wave.fp)) return _wave_error("truncated bit depth");

	if (memcmp(wave.header.riff, "RIFF", 4) != 0) return _wave_error("not a RIFF file");
	if (memcmp(wave.header.wave_flag, "WAVE", 4) != 0) return _wave_error("not a WAVE file");
	if (wave.header.tag != 1) return _wave_error("only PCM WAV is supported");
	if (wave.header.channels != 1 && wave.header.channels != 2) return _wave_error("only 1 or 2 channels are supported");
	if (wave.header.bit_samp != 32) return _wave_error("only 32-bit samples are supported");
	if (wave.header.byte_rate == 0) return _wave_error("invalid byte rate");

	// 跳过 fmt 块中超出标准 16 字节的部分
	uint32 remaining;
	if (wave.header.fmt_len >= 16) {
		remaining = wave.header.fmt_len - 16;
	} else {
		return _wave_error("invalid fmt chunk length");
	}
	while (remaining-- > 0) {
		if (fgetc(wave.fp) == EOF) return _wave_error("truncated fmt chunk");
	}

	// 定位 "data" 块，遇到文件结尾立即停止
	for (;;) {
		int c = fgetc(wave.fp);
		if (c == EOF) return _wave_error("no data chunk found");
		if (c != 'd') continue;
		wave.data_flag[0] = 'd';
		if (!_read_exact(&wave.data_flag[1], 3, wave.fp)) return _wave_error("truncated data chunk");
		if (memcmp(wave.data_flag, "data", 4) != 0) return _wave_error("unexpected chunk");
		if (!_read_exact(&wave.length, 4, wave.fp)) return _wave_error("truncated data length");
		break;
	}

	uint32 total_time = wave.length / wave.header.byte_rate;
	uint8 hour = (uint8)(total_time / 3600);
	uint8 minute = (uint8)((total_time / 60) % 60);
	uint8 second = (uint8)(total_time % 60);

	fprintf(stderr, "%s %uHz %ubit, DataLen: %u, Rate: %u, Length: %02u:%02u:%02u\n",
			wave.header.channels == 1 ? "mono" : "stereo",
			wave.header.samp_freq,
			wave.header.bit_samp,
			wave.length,
			wave.header.byte_rate,
			hour, minute, second);

	return 1;
}

#define WAVE_BUFFER_SIZE 2
static sint32 buffer[WAVE_BUFFER_SIZE] = {0};

/*
 * 读取一组波形数据
 */
int GetWave(void) {
	return fread(buffer, sizeof(sint32), WAVE_BUFFER_SIZE, wave.fp);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "usage wave file\n");
		return EXIT_FAILURE;
	}
	if (!WaveOpen(argv[1])) {
		return EXIT_FAILURE;
	}

	int r;
	while ((r = GetWave()) != 0) {
		fwrite(buffer, sizeof(sint32), r, stdout);
	}

	fclose(wave.fp);
	return EXIT_SUCCESS;
}
