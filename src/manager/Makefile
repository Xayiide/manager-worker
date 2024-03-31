CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic -O0 -g3 
#LFLAGS = -lcurl

TRGT = manager
SRCS = manager.c manager_sm.c
OBJS = $(SRCS:.c=.o)

.PHONY: all run clean re

all: $(TRGT)

$(TRGT): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -c
	$(CC) $(CFLAGS) $(OBJS) -o $@

run: all
	@./$(TRGT) localhost 6000

clean:
	@rm -f $(TRGT) $(OBJS)
	@echo "clean"

re: clean all
