CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200112L

#Target executable name
TARGET = udpchat

#Source files y object files
SRCS = udpchat.c
OBJS = $(SRCS:.c=.o)

#Default target: build exe
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHNOY: all clean