(*
 * Copyright (C) 2006-2014 Citrix Inc.
 * Copyright (c) 2010 Anil Madhavapeddy <anil@recoil.org>
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

external init: unit -> handle = "stub_evtchn_init"
external close: handle -> int = "stub_evtchn_close"

type t = int Generation.t

external stub_bind_unbound_port: handle -> int -> int = "stub_evtchn_alloc_unbound"
external stub_bind_interdomain: handle -> int -> int -> int = "stub_evtchn_bind_interdomain"
external stub_unmask: handle -> int -> unit = "stub_evtchn_unmask"
external stub_notify: handle -> int -> unit = "stub_evtchn_notify" "noalloc"
external stub_unbind: handle -> int -> unit = "stub_evtchn_unbind"
external stub_virq_dom_exc: unit -> int = "stub_evtchn_virq_dom_exc"
external stub_bind_virq: handle -> int -> int = "stub_evtchn_bind_virq"

let construct f x = Generation.wrap (f x)
let bind_unbound_port h = construct (stub_bind_unbound_port h)
let bind_interdomain h remote_domid = construct (stub_bind_interdomain h remote_domid)

let maybe t f d = Generation.maybe t f d
let unmask h t = maybe t (stub_unmask h) ()
let notify h t = maybe t (stub_notify h) ()
let unbind h t = maybe t (stub_unbind h) ()
let is_valid t = maybe t (fun _ -> true) false

let of_int n = Generation.wrap n
let to_int t = Generation.extract t

let bind_dom_exc_virq h =
  let port = stub_bind_virq h (stub_virq_dom_exc ()) in
  construct (fun () -> port) ()
