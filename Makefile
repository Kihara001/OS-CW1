CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = monitor.exe
OBJS = monitor.o utils.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

monitor.o: monitor.c utils.h
	$(CC) $(CFLAGS) -c monitor.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
