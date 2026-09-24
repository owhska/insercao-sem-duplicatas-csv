CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c99
TARGET  = src/copiador
SRC     = src/copiador.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
