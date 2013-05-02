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

(** Allow a user-space program running on Linux/xen to read/write
    memory exported ("granted") from foreign domains. Safe memory
    sharing is a building block of all xen inter-domain communication
    protocols such as those for virtual network and disk devices.

    Foreign domains will explicitly "grant" us access to certain memory
    regions such as disk buffers. These regions are uniquely identified
    by the pair of (foreign domain id, integer reference) which is
    passed to us over some existing channel (typically via xenstore keys
    or via structures in previously-shared memory region).

    A xen-aware kernel will use hypercalls to cause xen to map the
    foreign memory. In Linux userspace we must open a connection to the
    "grant device driver" to request the kernel ask xen to map foreign memory
    regions into our address space. Once mapped, we can access the raw
    memory contents as a Bigarray. In the case of a disk buffer, we will
    fill the buffers with disk blocks, unmap the page and then signal
    the foreign domain that we're finished, normally via an event channel.
*) 

(** {0 This is a low-level, unsafe API}
    This is a one-to-one mapping of the underlying C functions. *)

type buf = (char, Bigarray.int8_unsigned_elt, Bigarray.c_layout) Bigarray.Array1.t

type interface
(** A connection to the grant device, needed for mapping/unmapping *)

val interface_open: unit -> interface
(** Open a connection to the grant device. This must be done before any
    calls to map or unmap. *)

val interface_close: interface -> unit
(** Close a connection to the grant device. Any future calls to map or
    unmap will fail. *)

type grant_table_index
(** Abstract type of grant table index. *)

val grant_table_index_of_int32: int32 -> grant_table_index
val int32_of_grant_table_index: grant_table_index -> int32
val string_of_grant_table_index: grant_table_index -> string

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
| RO  (** contents may only be read *)
| RW  (** contents may be read and written *)
(** Permissions associated with each mapping. *)

module Local_mapping : sig 
  type t
  (** Abstract type representing a locally-mapped shared memory page *)

  val to_buf: t -> buf
end 

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
