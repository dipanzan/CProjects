# Compile with:   gcc -O2 -Wall -o rapl-read rapl-read.c -lm

CC = gcc
CFLAGS = -O2 -g -lm
TARGET = rapl

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)

perf:
	sudo ./rapl -p -1
