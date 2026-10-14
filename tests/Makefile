CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror
LDLIBS ?= -lm

.PHONY: all clean test

all: dsp_boundary_test

dsp_boundary_test: dsp_boundary_test.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

test: dsp_boundary_test
	./dsp_boundary_test

clean:
	$(RM) dsp_boundary_test
