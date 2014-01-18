(*
 * Copyright (c) 2010 Anil Madhavapeddy <anil@recoil.org>
 * Copyright (C) 2012-2014 Citrix Inc
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *)

open Lwt

type gntref = int
type domid = int

let console = 0 (* public/grant_table.h:GNTTAB_RESERVED_CONSOLE *)
let xenstore = 1 (* public/grant_table.h:GNTTAB_RESERVED_XENSTORE *)

type grant_handle (* handle to a mapped grant *)

module Gnttab = struct
  type interface

  external interface_open: unit -> interface = "stub_gnttab_interface_open"
  external interface_close: interface -> unit = "stub_gnttab_interface_close"

  type grant = {
    domid: domid;
    ref: gntref;
  }

  module Local_mapping = struct
    type t = {
      h : grant_handle;
      pages: Io_page.t;
    }

    let make h pages = { h; pages }

    let to_buf t = t.pages
  end

  external map_exn: interface -> gntref -> domid -> bool -> (grant_handle * Io_page.t) = "stub_gnttab_map"

  let map_exn interface grant writeable =
    let h, page = map_exn interface grant.ref grant.domid (not writeable) in
    Local_mapping.make h page

  let map interface grant writable = try Some (map_exn interface grant writable) with _ -> None

  external mapv_exn: interface -> int array -> bool -> (grant_handle * Io_page.t) = "stub_gnttab_mapv"

  let mapv_exn interface gs p =
    let count = List.length gs in
    let grant_array = Array.create (count * 2) 0 in
    List.iteri (fun i g ->
      grant_array.(i * 2 + 0) <- g.domid;
      grant_array.(i * 2 + 1) <- g.ref;
    ) gs;
    let h, page = mapv_exn interface grant_array p in
    Local_mapping.make h page

  let mapv interface gs p = try Some (mapv_exn interface gs p) with _ -> None

  external unmap_exn : interface -> grant_handle -> unit = "stub_gnttab_unmap"

  let unmap_exn interface mapping = unmap_exn interface mapping.Local_mapping.h

  let with_gnttab f =
    let intf = interface_open () in
    let result = try
      f intf
    with e ->
      interface_close intf;
      raise e
    in
    interface_close intf;
    result

  let with_mapping interface grant writeable fn =
    let mapping = map interface grant writeable in
    try_lwt fn mapping
    finally
      match mapping with
      | None -> Lwt.return ()
      | Some mapping -> Lwt.return (unmap_exn interface mapping)
end

module Gntshr = struct
  type interface

  external interface_open: unit -> interface = "stub_gntshr_open"
  external interface_close: interface -> unit = "stub_gntshr_close"

  type share = {
    refs: gntref list;
    mapping: Io_page.t;
  }

  module Lowlevel = struct
    exception Interface_unavailable

    type interface

    external interface_open: unit -> interface = "stub_gntshr_lowlevel_open"
    external interface_close: interface -> unit = "stub_gntshr_lowlevel_close"

    (* For kernelspace we need to track the real free grant table slots. *)

    let free_list : gntref Queue.t = Queue.create ()
    let free_list_waiters = Lwt_sequence.create ()

    let put r =
      Queue.push r free_list;
      match Lwt_sequence.take_opt_l free_list_waiters with
      | None -> ()
      | Some u -> Lwt.wakeup u ()

    let num_free_grants interface = Queue.length free_list

    let rec get interface =
      match Queue.is_empty free_list with
      | true ->
        let th, u = Lwt.task () in
        let node = Lwt_sequence.add_r u free_list_waiters  in
        Lwt.on_cancel th (fun () -> Lwt_sequence.remove node);
        th >> get interface
      | false ->
        return (Queue.pop free_list)

    let get_n interface num =
      let rec gen_gnts num acc =
      match num with
      | 0 -> return acc
      | n ->
        lwt gnt = get interface in
        gen_gnts (n-1) (gnt :: acc)
      in gen_gnts num []

    let get_nonblock interface =
      try Some (Queue.pop free_list) with Queue.Empty -> None

    let get_n_nonblock interface num =
      let rec aux acc num = match num with
      | 0 -> List.rev acc
      | n ->
        (match get_nonblock interface with
         | Some p -> aux (p::acc) (n-1)
           (* If we can't have enough, we push them back in the queue. *)
         | None -> List.iter (fun gntref -> Queue.push gntref free_list) acc; [])
        in aux [] num

    let with_ref interface f =
      lwt gnt = get interface in
      try_lwt f gnt
      finally Lwt.return (put gnt)

    let with_refs interface n f =
      lwt gnts = get_n interface n in
      try_lwt f gnts
      finally Lwt.return (List.iter put gnts)

    external grant_access : interface -> gntref -> Io_page.t -> int -> bool -> unit = "stub_gnttab_grant_access"

    let grant_access ~interface ~domid ~writeable gntref page = grant_access interface gntref page domid (not writeable)

    external end_access : interface -> gntref -> unit = "stub_gnttab_end_access"

    let with_grant ~interface ~domid ~writeable gnt page fn =
      grant_access ~interface ~domid ~writeable gnt page;
      try_lwt fn ()
      finally Lwt.return (end_access interface gnt)

    let with_grants ~interface ~domid ~writeable gnts pages fn =
      try_lwt
        List.iter (fun (gnt, page) ->
          grant_access ~interface ~domid ~writeable gnt page) (List.combine gnts pages);
          fn ()
      finally
        Lwt.return (List.iter (end_access interface) gnts)

    exception Grant_table_full

    let share_pages_exn interface domid count writeable =
      (* First allocate a list of n pages. *)
      let block = Io_page.get count in
      let pages = Io_page.to_pages block in
      let gntrefs = get_n_nonblock interface count in
      if gntrefs = []
      then raise Grant_table_full
      else begin
        List.iter2 (fun g p -> grant_access ~interface ~domid ~writeable g p) gntrefs pages;
        { refs = gntrefs; mapping = block }
      end

    (* Let the C stubs decide which function to call *)
    let () = Callback.register "share_pages_exn" share_pages_exn

    let munmap_exn interface { refs; _ } =
      List.iter (end_access interface) refs

    let () = Callback.register "munmap_exn" munmap_exn
  end

  external share_pages_exn: interface -> int -> int -> bool -> share = "stub_gntshr_share_pages"

  exception Need_xen_4_2_or_later

  let () = Callback.register_exception "gntshr.missing" Need_xen_4_2_or_later

  external munmap_exn: interface -> share -> unit = "stub_gntshr_munmap"

  let share_pages interface domid count writeable =
    try Some (share_pages_exn interface domid count writeable)
    with _ -> None

  let with_gntshr f =
    let intf = interface_open () in
    let result = try
      f intf
    with e ->
      interface_close intf;
      raise e
    in
    interface_close intf;
    result
end


external suspend : unit -> unit = "stub_gnttab_fini"

external init : unit -> unit = "stub_gnttab_init"

let resume () = init ()

external nr_entries : unit -> int = "stub_gnttab_nr_entries"
external nr_reserved : unit -> int = "stub_gnttab_reserved"

let _ =
  for i = nr_reserved () to nr_entries () - 1 do
    Gntshr.Lowlevel.put i;
  done;
  init ()

