CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lpthread
TARGET = pdp11emu
OBJECTS = main.o pdp11.o disk.o serial.o telnet.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

main.o: main.c pdp11.h disk.h serial.h telnet.h
	$(CC) $(CFLAGS) -c main.c

pdp11.o: pdp11.c pdp11.h
	$(CC) $(CFLAGS) -c pdp11.c

disk.o: disk.c disk.h
	$(CC) $(CFLAGS) -c disk.c

serial.o: serial.c serial.h
	$(CC) $(CFLAGS) -c serial.c

telnet.o: telnet.c telnet.h
	$(CC) $(CFLAGS) -c telnet.c

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

.PHONY: all clean install
