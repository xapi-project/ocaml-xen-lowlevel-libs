(*
 * Copyright (C) 2012-2013 Citrix Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *)

type share = {
	references: int32 list;
	(** Grant table references of the shared pages. *)
	mapping: Gntcommon.mapping;
	(** Mapping to the shared memory. *)
}

exception Need_xen_4_2_or_later
(** The needed low-level functions are only in xen >= 4.2 *)

type handle

external interface_open : unit -> handle = "stub_xenctrlext_gntshr_open"
external interface_close : handle -> unit = "stub_xenctrlext_gntshr_close"

external share_pages : handle -> int32 -> int -> bool -> share =
	"stub_xenctrlext_gntshr_share_pages"
external munmap : handle -> share -> unit =
	"stub_xenctrlext_gntshr_munmap"
