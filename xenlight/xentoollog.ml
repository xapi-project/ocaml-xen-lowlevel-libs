(*
 * Copyright (C) 2012      Citrix Ltd.
 * Author Ian Campbell <ian.campbell@citrix.com>
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

open Printf
open Random
open Callback

type level = Debug
	| Verbose
	| Detail
	| Progress
	| Info
	| Notice
	| Warn
	| Error
	| Critical

let level_to_string level =
	match level with
	| Debug -> "Debug"
	| Verbose -> "Verbose"
	| Detail -> "Detail"
	| Progress -> "Progress"
	| Info -> "Info"
	| Notice -> "Notice"
	| Warn -> "Warn"
	| Error -> "Error"
	| Critical -> "Critical"

let level_to_prio level = 
	match level with
	| Debug -> 0
	| Verbose -> 1
	| Detail -> 2
	| Progress -> 3
	| Info -> 4
	| Notice -> 5
	| Warn -> 6
	| Error -> 7
	| Critical -> 8

let compare_level x y =
	compare (level_to_prio x) (level_to_prio y)

type handle

type logger_cbs = {
	vmessage : level -> int option -> string option -> string -> unit;
	progress : string option -> string -> int -> int64 -> int64 -> unit;
	(*destroy : unit -> unit*)
}

external _create_logger: (string * string) -> handle = "stub_xtl_create_logger"
external test: handle -> unit = "stub_xtl_test"

let create name cbs : handle =
	(* Callback names are supposed to be unique *)
	let suffix = string_of_int (Random.int 1000000) in
	let vmessage_name = sprintf "%s_vmessage_%s" name suffix in
	let progress_name = sprintf "%s_progress_%s" name suffix in
	(*let destroy_name = sprintf "%s_destroy" name in*)
	Callback.register vmessage_name cbs.vmessage;
	Callback.register progress_name cbs.progress;
	_create_logger (vmessage_name, progress_name)


let stdio_vmessage min_level level errno ctx msg =
	let level_str = level_to_string level
	and errno_str = match errno with None -> "" | Some s -> sprintf ": errno=%d" s
	and ctx_str = match ctx with None -> "" | Some s -> sprintf ": %s" s in
	if compare min_level level <= 0 then begin
		printf "%s%s%s: %s\n" level_str ctx_str errno_str msg;
		flush stdout;
	end

let stdio_progress ctx what percent dne total =
	let nl = if dne = total then "\n" else "" in
	printf "\rProgress %s %d%% (%Ld/%Ld)%s" what percent dne total nl;
	flush stdout

let create_stdio_logger ?(level=Info) () =
	let cbs = {
		vmessage = stdio_vmessage level;
		progress = stdio_progress; } in
	create "Xentoollog.stdio_logger" cbs

