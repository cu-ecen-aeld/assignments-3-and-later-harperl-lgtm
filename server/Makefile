CROSS_COMPILE ?=

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -g
TARGET = aesdsocket

all:$(TARGET)

default: $(TARGET)

$(TARGET): aesdsocket.c
    # compiles the first prerequisite file $< (aesdsocket.c) into object file $@ (aesdsocket)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -f *.o