SUBDIRS = manager worker net

.PHONY: all run clean re $(SUBDIRS)

all: $(SUBDIRS)

net:
	$(MAKE) -C net

manager: net
	cp net/net.o manager/

worker: net
	cp net/net.o worker/

run: all
	@./manager/manager
	@./worker/worker

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@; \
	done

re: clean all
