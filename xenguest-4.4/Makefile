CFLAGS += -Werror
CFLAGS += -I.
CFLAGS += -lxenctrl -lxenguest -lxenstore
CFLAGS += -D_GNU_SOURCE

PROGRAMS := xenguest

.PHONY: all
all: build

.PHONY: build
build: $(PROGRAMS)

xenguest: xenguest.o xenguest_stubs.o
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^ \
		-lxenctrl -lxenguest -lxenstore
	rm *.o

.PHONY: install
install:
	mkdir -p $(BINDIR)
	install -m 0755 xenguest $(BINDIR)

.PHONY: uninstall
	rm -f $(BINDIR)/xenguest

.PHONY: clean
clean:
	rm -f *.o $(PROGRAMS)
