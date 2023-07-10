CC = mpicc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -L$(HOME)/gmp-install/lib -lgmp

TARGET = Factorial
SRCS = Factorial.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(HOME)/gmp-install/include -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

