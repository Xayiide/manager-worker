SUBDIRS = manager worker net

.PHONY: all run clean re $(SUBDIRS)

all: $(SUBDIRS)

manager:
	$(MAKE) -C net
	cp net/net.o manager/

net:
	$(MAKE) -C net

worker:
	$(MAKE) -C net
	cp net/net.o worker/
	

run: all
	@./manager/manager
	@./worker/worker

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@; \
	done

re: clean all
