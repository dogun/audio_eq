CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror
LDLIBS ?= -lm

.PHONY: all clean test

all: dsp_boundary_test loudness_test

dsp_boundary_test: dsp_boundary_test.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

loudness_test: loudness_test.c stubs/esp_log.h
	$(CC) $(CFLAGS) -Istubs -Wno-unused-function -Wno-unused-variable $< $(LDLIBS) -o $@

test: dsp_boundary_test loudness_test
	./dsp_boundary_test
	./loudness_test

clean:
	$(RM) dsp_boundary_test loudness_test
