ocaml-xen-lowlevel-libs
=======================

[![Build Status](https://travis-ci.org/xapi-project/ocaml-xen-lowlevel-libs.svg?branch=master)](https://travis-ci.org/xapi-project/ocaml-xen-lowlevel-libs)

This repo contains the OCaml code from
[xen.git](http://xenbits.xen.org/gitweb/?p=xen.git;a=summary)
with some autodetection code to build the version that corresponds to
the Xen headers on your system. This repo is useful to build Xen-related
code from opam packages, since the OCaml libraries being used under opam
will probably be different to the system installed versions.

