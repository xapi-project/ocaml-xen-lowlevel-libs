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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *)

type buf = (char, Bigarray.int8_unsigned_elt, Bigarray.c_layout) Bigarray.Array1.t

type grant_table_index
(** Abstract type of grant table index. *)

val grant_table_index_of_int32: int32 -> grant_table_index
val int32_of_grant_table_index: grant_table_index -> int32

val grant_table_index_of_string: string -> grant_table_index
val string_of_grant_table_index: grant_table_index -> string

module Local_mapping : sig
	type t
	(** Abstract type representing a locally-mapped shared memory page *)

	val to_buf: t -> buf
end

module Gnttab : sig
	type interface
	(** A connection to the grant device, needed for mapping/unmapping *)

	val interface_open: unit -> interface
	(** Open a connection to the grant device. This must be done before any
	    calls to map or unmap. *)

	val interface_close: interface -> unit
	(** Close a connection to the grant device. Any future calls to map or
	    unmap will fail. *)

	type grant = {
		domid: int;
		(** foreign domain who is exporting memory *)
		ref: grant_table_index;
		(** id which identifies the specific export in the foreign domain *)
	}
	(** A foreign domain must explicitly "grant" us memory and send us the
	    "reference". The pair of (foreign domain id, reference) uniquely
	    identifies the block of memory. This pair ("grant") is transmitted
	    to us out-of-band, usually either via xenstore during device setup or
	    via a shared memory ring structure. *)

	type permission =
		| RO (** contents may only be read *)
		| RW (** contents may be read and written *)
		(** Permissions associated with each mapping. *)

	val map: interface -> grant -> permission -> Local_mapping.t option
	(** Create a single mapping from a grant using a given list of permissions.
	    On error this function returns None. Diagnostic details will be logged. *) 

	val mapv: interface -> grant list -> permission -> Local_mapping.t option
	(** Create a single contiguous mapping from a list of grants using a common
	    list of permissions. Note the grant list can involve grants from multiple
	    domains. On error this function returns None. Diagnostic details will
	    be logged. *)

	val unmap_exn: interface -> Local_mapping.t -> unit
	(** Unmap a single mapping (which may involve multiple grants) *)
end

module Gntshr : sig
	type interface

	external interface_open: unit -> interface = "stub_xenctrlext_gntshr_open"
	external interface_close: interface -> unit = "stub_xenctrlext_gntshr_close"

	type share = {
		refs: grant_table_index list;
		mapping: Local_mapping.t;
	}

	external share_pages: interface -> int32 -> int -> bool -> share =
		"stub_xenctrlext_gntshr_share_pages"
	external munmap : interface -> share -> unit =
		"stub_xenctrlext_gntshr_munmap"
end
