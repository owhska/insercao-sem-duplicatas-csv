CC      = gcc
CFLAGS  = -O2 -fno-optimize-sibling-calls -Wall -Wextra -std=c99

all: src/copiador src/bench

src/copiador: src/copiador.c src/algoritmos.c src/algoritmos.h
	$(CC) $(CFLAGS) -o src/copiador src/copiador.c src/algoritmos.c

src/bench: src/bench.c src/algoritmos.c src/algoritmos.h
	$(CC) $(CFLAGS) -o src/bench src/bench.c src/algoritmos.c

clean:
	rm -f src/copiador src/copiador.exe src/bench src/bench.exe

.PHONY: all clean
