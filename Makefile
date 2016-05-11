.PHONY: all clean install build
all: build doc

BINDIR?=/usr/lib/xcp/lib

J=4

include config.mk
config.mk:
	echo Please re-run configure
	exit 1

BINDIR ?= /usr/bin
SBINDIR ?= /usr/sbin
DESTDIR ?= /

export OCAMLRUNPARAM=b

setup.bin: setup.ml
	@ocamlopt.opt -o $@ $< || ocamlopt -o $@ $< || ocamlc -o $@ $<
	@rm -f setup.cmx setup.cmi setup.o setup.cmo

setup.data: setup.bin config.mk
	@./setup.bin -configure $(ENABLE_XENGUEST42)

build: setup.data setup.bin
	@./setup.bin -build -j $(J)
ifeq ($(ENABLE_XENGUEST44),true)
	(cd xenguest-4.4 && make)
endif
ifeq ($(ENABLE_XENGUEST46),true)
	(cd xenguest-4.6 && make)
endif

doc: setup.data setup.bin
	@./setup.bin -doc -j $(J)

install: setup.bin
	@./setup.bin -install
ifeq ($(ENABLE_XENGUEST42),--enable-xenguest42)
	mkdir -p $(BINDIR)
	install -m 0755 _build/xenguest-4.2/xenguest_main.native $(BINDIR)/xenguest
endif
ifeq ($(ENABLE_XENGUEST44),true)
	(cd xenguest-4.4 && make install BINDIR=$(BINDIR))
endif
ifeq ($(ENABLE_XENGUEST46),true)
	(cd xenguest-4.6 && make install BINDIR=$(BINDIR))
endif

test: setup.bin build
	@./setup.bin -test

reinstall: setup.bin
	@ocamlfind remove xenctrl || true
	@ocamlfind remove xenlight || true
	@./setup.bin -reinstall
ifeq ($(ENABLE_XENGUEST44),true)
	(cd xenguest-4.4 && make install BINDIR=$(BINDIR))
endif
ifeq ($(ENABLE_XENGUEST46),true)
	(cd xenguest-4.6 && make install BINDIR=$(BINDIR))
endif

uninstall:
	@ocamlfind remove xenctrl || true
	@ocamlfind remove xenlight || true
	@ocamlfind remove xentoollog || true
ifeq ($(ENABLE_XENGUEST44),true)
	(cd xenguest-4.4 && make uninstall BINDIR=$(BINDIR))
endif
ifeq ($(ENABLE_XENGUEST46),true)
	(cd xenguest-4.6 && make uninstall BINDIR=$(BINDIR))
endif

clean:
	@ocamlbuild -clean
	@rm -f setup.data setup.log setup.bin
ifeq ($(ENABLE_XENGUEST44),true)
	(cd xenguest-4.4 && make clean)
endif
ifeq ($(ENABLE_XENGUEST46),true)
	(cd xenguest-4.6 && make clean)
endif
