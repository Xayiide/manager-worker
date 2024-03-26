CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic -O0 -g3 
#IFLAGS = -I../include

TRGT = worker
SRCS = worker.c
OBJS = $(SRCS:.c=.o)

.PHONY: all run clean re

all: $(TRGT)

$(TRGT): $(SRCS)
	$(CC) $(CFLAGS) $(IFLAGS) $(SRCS) -c
	$(CC) $(CFLAGS) $(OBJS) $(LFLAGS) -o $@

run: all
	@./$(TRGT) localhost 6000

clean:
	@rm -f $(TRGT) $(OBJS)
	@echo "clean"

re: clean all
