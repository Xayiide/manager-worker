CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic -O0 -g3 
IFLAGS = -I../include
LFLAGS = -lcurl

TRGT = worker
SRCS = worker.c
OBJS = $(SRCS:.c=.o) net.o

.PHONY: all run clean re

all: $(TRGT)

$(TRGT): $(SRCS) net.o
	$(CC) $(CFLAGS) $(IFLAGS) $(SRCS) -c
	$(CC) $(CFLAGS) $(OBJS) $(LFLAGS) -o $@

net.o:
	$(MAKE) -C ../net
	cp ../net/net.o .

run: all
	@./$(TRGT) localhost 6000

clean:
	@rm -f $(TRGT) $(OBJS)
	@rm -f net.o
	@echo "clean"

re: clean all
