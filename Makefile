CC = gcc
CFLAGS = -Wall -Wextra -O2
OBJS = monitor.o utils.o
TARGET = monitor.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

monitor.o: monitor.c utils.h
	$(CC) $(CFLAGS) -c monitor.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
