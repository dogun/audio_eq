mkdir -p bin
gcc -o bin/record record.c -lasound
gcc -o bin/play play.c -lasound -lm
gcc -o bin/line line.c -lasound -lm
gcc -o bin/eq eq.c -lasound -lm
gcc -o bin/wave wave.c
gcc -o bin/cc cc.c -lm

