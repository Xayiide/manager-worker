CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic -Wconversion \
	 -Wno-sign-conversion -Wdouble-promotion -Wno-unused-parameter \
	 -Wno-unused-function -O0 -g3
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
