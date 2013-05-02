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

type buf = (char, Bigarray.int8_unsigned_elt, Bigarray.c_layout) Bigarray.Array1.t

type interface

external interface_open: unit -> interface = "stub_xc_gnttab_open"

external interface_close: interface -> unit = "stub_xc_gnttab_close"

type grant_table_index = int32
let grant_table_index_of_int32 x = x
let int32_of_grant_table_index x = x
let string_of_grant_table_index = Int32.to_string

type grant = {
    domid: int;
    ref: grant_table_index;
}

module Local_mapping = struct
  type t = buf 

  let to_buf t = t
end

external map_exn: interface -> int32 -> int32 -> int -> Local_mapping.t =
    "stub_xc_gnttab_map_grant_ref"
external mapv_exn: interface -> int32 array -> int -> Local_mapping.t =
    "stub_xc_gnttab_map_grant_refs"
external unmap_exn: interface -> Local_mapping.t -> unit =
    "stub_xc_gnttab_unmap"

(* Look up the values of PROT_{READ,WRITE} from the C headers. *)
type perm = PROT_READ | PROT_WRITE
external get_perm: perm -> int =
    "stub_xc_gnttab_get_perm"
let _PROT_READ = get_perm PROT_READ
let _PROT_WRITE = get_perm PROT_WRITE

type permission = RO | RW

let int_of_permission = function
| RO -> _PROT_READ
| RW -> _PROT_READ lor _PROT_WRITE

let map h g p =
    try
        Some (map_exn h (Int32.of_int g.domid) g.ref (int_of_permission p))
    with _ ->
        None

let mapv h gs p =
    try
        let count = List.length gs in
        let grant_array = Array.create (count * 2) 0l in
        let (_: int) = List.fold_left (fun i g ->
            grant_array.(i * 2 + 0) <- Int32.of_int g.domid;
            grant_array.(i * 2 + 1) <- g.ref;
            i + 1
        ) 0 gs in
        Some (mapv_exn h grant_array (int_of_permission p))
    with _ ->
        None
