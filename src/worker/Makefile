CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O0 -g3 
IFLAGS = -I../include

TRGT = worker
SRCS = worker.c
OBJS = $(SRCS:.c=.o) net.o

.PHONY: all run clean re

all: $(TRGT)

$(TRGT): $(SRCS) net.o
	$(CC) $(CFLAGS) $(IFLAGS) $(SRCS) -c
	$(CC) $(CFLAGS) $(OBJS) -o $@

net.o:
	$(MAKE) -C ../net
	cp ../net/net.o .

run: all
	@./$(TRGT)

clean:
	@rm -f $(TRGT) $(OBJS)
	@rm -f net.o
	@echo "clean"

re: clean all
