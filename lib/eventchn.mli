(*
 * Copyright (C) 2006-2007 XenSource Ltd.
 * Copyright (C) 2008      Citrix Ltd.
 * Author Vincent Hanquez <vincent.hanquez@eu.citrix.com>
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

exception Error of string

type handle

val init : unit -> handle
val fd: handle -> Unix.file_descr

val notify : handle -> int -> unit
val bind_interdomain : handle -> int -> int -> int

val bind_dom_exc_virq : handle -> int
val unbind : handle -> int -> unit
val pending : handle -> int
val unmask : handle -> int -> unit

