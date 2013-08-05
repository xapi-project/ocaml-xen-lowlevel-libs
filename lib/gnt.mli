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

(** Grant tables interface. *)

type buf = (char, Bigarray.int8_unsigned_elt, Bigarray.c_layout) Bigarray.Array1.t

type gntref = int
(** Type of a grant table index, called a grant reference in
    Xen's terminology. *)

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
   ref: gntref;
   (** id which identifies the specific export in the foreign domain *)
  }
  (** A foreign domain must explicitly "grant" us memory and send us the
      "reference". The pair of (foreign domain id, reference) uniquely
      identifies the block of memory. This pair ("grant") is transmitted
      to us out-of-band, usually either via xenstore during device setup or
      via a shared memory ring structure. *)

  module Local_mapping : sig
    type t
    (** Abstract type representing a locally-mapped shared memory page *)

    val to_buf: t -> buf
  end

  val map_exn : interface -> grant -> bool -> Local_mapping.t
  (** [map_exn if grant writable] creates a single mapping from
      [grant] that will be writable if [writable] is [true]. *)

  val map : interface -> grant -> bool -> Local_mapping.t option
  (** Like the above but wraps the result in an option instead of
      raising an exception. *)

  val mapv_exn : interface -> grant list -> bool -> Local_mapping.t
  (** [mapv if grants writable] creates a single contiguous mapping
      from a list of grants that will be writable if [writable] is
      [true]. Note the grant list can involve grants from multiple
      domains. *)

  val mapv: interface -> grant list -> bool -> Local_mapping.t option
  (** Like the above but wraps the result in an option instead of
      raising an exception. *)

  val unmap_exn: interface -> Local_mapping.t -> unit
  (** Unmap a single mapping (which may involve multiple grants) *)
end

module Gntshr : sig
  type interface
  (** A connection to the gntshr device, needed for sharing/unmapping *)

  val interface_open: unit -> interface
  (** Open a connection to the gntshr device. This must be done before any
      calls to share or unmap. *)

  val interface_close: interface -> unit
  (** Close a connection to the gntshr device. Any future calls to share or
      unmap will fail. *)

  type share = {
    refs: gntref list;
    (** List of grant references which have been shared with a foreign domain. *)
    mapping: buf;
    (** Mapping of the shared memory. *)
  }
  (** When sharing a number of pages with another domain, we receive back both the
      list of grant references shared and actually mapped page(s). The foreign
      domain can map the same shared memory, after being notified (e.g. via xenstore)
      of our domid and list of references. *)

  val share_pages_exn: interface -> int -> int -> bool -> share
  (** [share_pages_exn if domid count writeable] shares [count] pages with foreign
      domain [domid]. [writeable] determines whether or not the foreign domain can
      write to the shared memory. *)

  val share_pages: interface -> int -> int -> bool -> share option
	(** [share_pages if domid count writeable] shares [count] pages with foreign domain
     [domid]. [writeable] determines whether or not the foreign domain can write to
     the shared memory.
     On error this function returns None. Diagnostic details will be logged. *)

  val munmap_exn : interface -> share -> unit
  (** Unmap a single mapping (which may involve multiple grants) *)

  val get_n : int -> gntref list Lwt.t
  (** Allocate a block of n grant table indices *)
end
