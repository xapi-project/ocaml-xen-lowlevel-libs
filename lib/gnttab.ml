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

type handle

external interface_open: unit -> handle = "stub_xc_gnttab_open"

external interface_close: handle -> unit = "stub_xc_gnttab_close"

type grant = {
    domid: int32;
    reference: int32;
}

type contents = (char, Bigarray.int8_unsigned_elt, Bigarray.c_layout) Bigarray.Array1.t

type mapping = contents

let contents x = x

type permission = READ | WRITE
external map_exn: handle -> int32 -> int32 -> int -> contents =
    "stub_xc_gnttab_map_grant_ref"
external mapv_exn: handle -> int32 array -> int -> contents =
    "stub_xc_gnttab_map_grant_refs"
external unmap_exn: handle -> contents -> unit =
    "stub_xc_gnttab_unmap"

(* Look up the values of PROT_{READ,WRITE} from the C headers. *)
external get_perm: permission -> int =
    "stub_xc_gnttab_get_perm"
let _PROT_READ = get_perm READ
let _PROT_WRITE = get_perm WRITE

let int_of_permission = function
| READ -> _PROT_READ
| WRITE -> _PROT_WRITE

(* Convert a list of permissions to ints and or them together *)
let int_of_permissions ps = List.fold_left (lor) 0 (List.map int_of_permission ps)

let map h g ps =
    try
        Some (map_exn h g.domid g.reference (int_of_permissions ps))
    with _ ->
        None

let mapv h gs ps =
    try
        let count = List.length gs in
        let grant_array = Array.create (count * 2) 0l in
        let (_: int) = List.fold_left (fun i g ->
            grant_array.(i * 2 + 0) <- g.domid;
            grant_array.(i * 2 + 1) <- g.reference;
            i + 1
        ) 0 gs in
        Some (mapv_exn h grant_array (int_of_permissions ps))
    with _ ->
        None
