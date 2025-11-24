CROSS_COMPILE ?=

CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Wextra -g

TARGET = writer

SRCS = writer.c

OBJS = $(SRCS:.c=.o)

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)