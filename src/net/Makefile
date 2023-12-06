CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic -O0 -g3 

SRCS = net.c
OBJS = $(SRCS:.c=.o)


.PHONY: all clean re

all: $(OBJS)

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)

clean:
	@rm -f $(OBJS)
	@echo "clean"

re: clean all
