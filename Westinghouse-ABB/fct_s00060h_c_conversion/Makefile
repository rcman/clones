# S00060-H FCT Makefile
# Westinghouse Field Configuration Terminal - C conversion

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
DEBUG_FLAGS = -g -DDEBUG

# Source files
SRCS = fct_s00060h_part1.c fct_s00060h_part2.c fct_s00060h_part3.c fct_hal.c
HDRS = fct_s00060h.h

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = fct_simulator

# Default target
all: $(TARGET)

# Build with test main
$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DTEST_MAIN -o $@ $(SRCS)

# Debug build
debug: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -DTEST_MAIN -o $(TARGET)_debug $(SRCS)

# Build object files only (for linking into other projects)
objects: $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET)_debug

# Install (copy to /usr/local/bin)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all debug objects clean install uninstall
