.PHONY: all clean install build
all: build doc

BINDIR?=/usr/lib/xcp/lib

J=4

include config.mk
config.mk: configure
	./configure

configure: configure.ml
	ocamlfind ocamlopt -package "cmdliner" -linkpkg $< -o $@

BINDIR ?= /usr/bin
SBINDIR ?= /usr/sbin
DESTDIR ?= /

export OCAMLRUNPARAM=b

setup.bin: setup.ml
	@ocamlopt.opt -o $@ $< || ocamlopt -o $@ $< || ocamlc -o $@ $<
	@rm -f setup.cmx setup.cmi setup.o setup.cmo

setup.data: setup.bin config.mk
	@./setup.bin -configure $(ENABLE_XENLIGHT) $(ENABLE_XENCTRL)

build: setup.data setup.bin
	@./setup.bin -build -j $(J)
	(cd xenguest-4.4 && make)

doc: setup.data setup.bin
	@./setup.bin -doc -j $(J)

install: setup.bin
	@./setup.bin -install
	(cd xenguest-4.4 && make install BINDIR=$(BINDIR))

test: setup.bin build
	@./setup.bin -test

reinstall: setup.bin
	@ocamlfind remove xenctrl || true
	@ocamlfind remove xenlight || true
	@./setup.bin -reinstall
	(cd xenguest-4.4 && make install BINDIR=$(BINDIR))

uninstall:
	@ocamlfind remove xenctrl || true
	@ocamlfind remove xenlight || true
	(cd xenguest-4.4 && make uninstall BINDIR=$(BINDIR))

clean:
	@ocamlbuild -clean
	@rm -f setup.data setup.log setup.bin
	(cd xenguest-4.4 && make clean)
