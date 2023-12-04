CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -O0 -g3 

SRCS = net.c
OBJS = $(SRCS:.c=.o)


.PHONY: all clean re

all: $(OBJS)

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)

clean:
	@rm -f $(TRGT) $(OBJS) net_test
	@echo "clean"

re: clean all
