.PHONY: all clean install build
all: build doc

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

setup.data: setup.bin
	@./setup.bin -configure $(ENABLE_XENLIGHT) $(ENABLE_XENCTRL)

build: setup.data setup.bin
	@./setup.bin -build -j $(J)

doc: setup.data setup.bin
	@./setup.bin -doc -j $(J)

install: setup.bin
	@./setup.bin -install

test: setup.bin build
	@./setup.bin -test

reinstall: setup.bin
	@ocamlfind remove xenctrl || true
	@ocamlfind remove xenlight || true
	@./setup.bin -reinstall

uninstall:
	@ocamlfind remove xenctrl || true
	@ocamlfind remove xenlight || true

clean:
	@ocamlbuild -clean
	@rm -f setup.data setup.log setup.bin
