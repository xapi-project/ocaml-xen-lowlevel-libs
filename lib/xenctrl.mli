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

(** Other bindings to libxc. *)

(** {2 Initialization functions} *)

type handle
(** Type of a libxc handle. Corresponding to libxc's [xc_interface*] *)

(** General error exception *)
exception Error of string

external interface_open : unit -> handle = "stub_xc_interface_open"
(** This function opens a handle to the hypervisor interface.  This
    function can be called multiple times within a single process.
    Multiple processes can have an open hypervisor interface at the same
    time.

    Note:
    After fork a child process must not use any opened xc interface
    handle inherited from their parent. They must open a new handle if
    they want to interact with xc.

    Each call to this function should have a corresponding call to
    [interface_close].

    This function can fail if the caller does not have superuser permission or
    if a Xen-enabled kernel is not currently running.

    @return a handle to the hypervisor interface
*)

external interface_close : handle -> unit = "stub_xc_interface_close"
(** This function closes an open hypervisor interface.

    This function can fail if the handle does not represent an open interface or
    if there were problems closing the interface.  In the latter case
    the interface is still closed.

    @param xch a handle to an open hypervisor interface
*)

val with_intf : (handle -> 'a) -> 'a
(** [with_intf f] calls [f] with a freshly opened handle as
    argument. *)


(** {2 Physical host query functions} *)

type physinfo_cap_flag = CAP_HVM | CAP_DirectIO

type physinfo = {
  threads_per_core : int;
  cores_per_socket : int;
  nr_cpus          : int;
  max_node_id      : int;
  cpu_khz          : int;
  total_pages      : nativeint;
  free_pages       : nativeint;
  scrub_pages      : nativeint;
  capabilities     : physinfo_cap_flag list;
  max_nr_cpus      : int; (** compile-time max possible number of nr_cpus *)
}

external physinfo : handle -> physinfo = "stub_xc_physinfo"
(** [physinfo xch] is the [physinfo] structure related to the running
    Xen instance. *)

external pcpu_info: handle -> int -> int64 array = "stub_xc_pcpu_info"
(** [pcpu_info xch i] is the cpu info array for the physical CPU
    [i]. *)


(** {2 Xen get/set functions} *)

type version = { major : int; minor : int; extra : string; }

type compile_info = {
  compiler : string;
  compile_by : string;
  compile_domain : string;
  compile_date : string;
}

external version : handle -> version = "stub_xc_version_version"
external version_compile_info : handle -> compile_info = "stub_xc_version_compile_info"
external version_changeset : handle -> string = "stub_xc_version_changeset"
external version_capabilities : handle -> string = "stub_xc_version_capabilities"

external sched_id : handle -> int = "stub_xc_sched_id"
(** [sched_id xch] is the id of the scheduler currently used by
    Xen. *)


(** {2 Domain related functions} *)

type domid = int
(** Type of a domain id. *)

type vcpuinfo = {
  online : bool;
  blocked : bool;
  running : bool;
  cputime : int64;
  cpumap : int32;
}

type domaininfo = {
  domid : domid;
  dying : bool;
  shutdown : bool;
  paused : bool;
  blocked : bool;
  running : bool;
  hvm_guest : bool;
  shutdown_code : int;
  total_memory_pages : nativeint;
  max_memory_pages : nativeint;
  shared_info_frame : int64;
  cpu_time : int64;
  nr_online_vcpus : int;
  max_vcpu_id : int;
  ssidref : int32;
  handle : int array;
}

type runstateinfo = {
  state : int32;
  missed_changes: int32;
  state_entry_time : int64;
  time0 : int64;
  time1 : int64;
  time2 : int64;
  time3 : int64;
  time4 : int64;
  time5 : int64;
}

type domain_create_flag = CDF_HVM | CDF_HAP

val domain_create : handle -> int32 -> domain_create_flag list -> string -> domid
(** [domain_create xch ssidref flags uuid] creates a domain and
    returns its id. *)

val domain_sethandle : handle -> domid -> string -> unit
(** [domain_sethandle xch domid uuid] sets [uuid] as the handle of
    [domid]. *)

external domain_max_vcpus : handle -> domid -> int -> unit = "stub_xc_domain_max_vcpus"
(** [domain_max_vcpus xch domid max] sets [max] to be the maximum
    number of vcpus that [domid] may create. *)

external domain_getinfo : handle -> domid -> domaininfo = "stub_xc_domain_getinfo"
(** [domain_getinfo xch domid] is the [domaininfo] record for
    [domid]. *)

val domain_getinfolist : handle -> domid -> domaininfo list
(** [domain_getinfolist xch domid] is the list of all the [domaininfo]
    records starting from [domid] included. *)

external domain_setmaxmem : handle -> domid -> int64 -> unit = "stub_xc_domain_setmaxmem"
(** [domain_setmaxmem xch domid max] sets the maximum memory usable by
    [domid] to [max]. *)

external domain_set_machine_address_size: handle -> domid -> int -> unit = "stub_xc_domain_set_machine_address_size"
external domain_get_machine_address_size: handle -> domid -> int = "stub_xc_domain_get_machine_address_size"

external shadow_allocation_set : handle -> domid -> int -> unit = "stub_shadow_allocation_set"
external shadow_allocation_get : handle -> domid -> int = "stub_shadow_allocation_get"

external domain_set_memmap_limit : handle -> domid -> int64 -> unit = "stub_xc_domain_set_memmap_limit"
external domain_memory_increase_reservation : handle -> domid -> int64 -> unit = "stub_xc_domain_memory_increase_reservation"
external map_foreign_range : handle -> domid -> int -> nativeint -> Xenmmap.mmap_interface = "stub_map_foreign_range"

external domain_get_pfn_list : handle -> domid -> nativeint -> nativeint array = "stub_xc_domain_get_pfn_list"
(** DEPRECATED.  Avoid using this, as it does not correctly account
    for PFNs without a backing MFN. *)



(** {3 Domain lifecycle ops} *)

type shutdown_reason = Poweroff | Reboot | Suspend | Crash | Halt

external domain_pause : handle -> domid -> unit = "stub_xc_domain_pause"
(** [domain_pause xch domid] pauses [domid]. A paused domain still
    exists in memory however it does not receive any timeslices from
    the hypervisor. *)

external domain_unpause : handle -> domid -> unit = "stub_xc_domain_unpause"
(** [domain_unpause xch domid] unpauses [domid]. The domain should
    have been previously paused. *)

external domain_shutdown : handle -> domid -> shutdown_reason -> unit = "stub_xc_domain_shutdown"
(** [domain_shutdown xch domid reason] shutdowns [domid] with
    [reason]. This is intended for use in fully-virtualized domains where
    this operation is analogous to the [sched_op] operations in a
    paravirtualized domain. The caller is expected to give the reason for
    the shutdown. *)

external domain_resume_fast : handle -> domid -> unit = "stub_xc_domain_resume_fast"
(** [domain_resume_fast xch domid] resumes [domid]. The domain should
    have been previously suspended. *)

external domain_destroy : handle -> domid -> unit = "stub_xc_domain_destroy"
(** [domain_destroy xch domid] destroys [domid]. Destroying a domain
    removes the domain completely from memory. This function should be
    called after sending the domain a SHUTDDOWN control message to free up
    the domain resources. *)


external watchdog : handle -> domid -> int32 -> int = "stub_xc_watchdog"
(** [watchdog xch domid timeout] turns on the watchdog for domain
    [domid] with timeout [timeout]. *)

(** {3 Domain scheduling} *)

type sched_control = { weight : int; cap : int; }

external sched_credit_domain_set : handle -> domid -> sched_control -> unit = "stub_sched_credit_domain_set"
external sched_credit_domain_get : handle -> domid -> sched_control = "stub_sched_credit_domain_get"

(** {3 Domain VCPU management} *)


external domain_get_vcpuinfo : handle -> domid -> int -> vcpuinfo = "stub_xc_vcpu_getinfo"
(** [domain_get_vcpuinfo xch domid v] is the [vcpuinfo] record for
    vcpu [v] of domain [domid]. *)

external vcpu_affinity_get : handle -> domid -> int -> bool array = "stub_xc_vcpu_getaffinity"
(** [vcpu_affinity_get xch domid v] is the affinity array for vcpu [v]
    in domain [domid]. For each physical CPUs [i], [affinity.(i)] will
    have the value [true] if [v] can be scheduled on [i]. *)

external vcpu_affinity_set : handle -> domid -> int -> bool array -> unit = "stub_xc_vcpu_setaffinity"
(** [vcpu_affinity_set xch domid v affinities] sets vcpu [v] of domain
    [domid] to be scheduled according to [affinities]. See the
    documentation of the previous function about the affinities
    array. *)

external vcpu_context_get : handle -> domid -> int -> string = "stub_xc_vcpu_context_get"


(** {3 HVM guest pass-through management} *)

external domain_ioport_permission: handle -> domid -> int -> int -> bool -> unit = "stub_xc_domain_ioport_permission"
external domain_iomem_permission: handle -> domid -> nativeint -> nativeint -> bool -> unit = "stub_xc_domain_iomem_permission"
external domain_irq_permission: handle -> domid -> int -> bool -> unit = "stub_xc_domain_irq_permission"

external domain_assign_device: handle -> domid -> (int * int * int * int) -> unit = "stub_xc_domain_assign_device"
external domain_deassign_device: handle -> domid -> (int * int * int * int) -> unit = "stub_xc_domain_deassign_device"
external domain_test_assign_device: handle -> domid -> (int * int * int * int) -> bool = "stub_xc_domain_test_assign_device"

external domain_get_runstate_info : handle -> int -> runstateinfo = "stub_xc_get_runstate_info"

(** {3 Domain eventchn functions} *)

external evtchn_alloc_unbound : handle -> domid -> domid -> int = "stub_xc_evtchn_alloc_unbound"
external evtchn_reset : handle -> domid -> unit = "stub_xc_evtchn_reset"


(** {3 Domain console functions} *)

external readconsolering : handle -> string = "stub_xc_readconsolering"
external send_debug_keys : handle -> string -> unit = "stub_xc_send_debug_keys"




(** {3 CPUID stuff} *)

external domain_cpuid_set : handle -> domid -> (int64 * (int64 option)) -> string option array -> string option array = "stub_xc_domain_cpuid_set"
external domain_cpuid_apply_policy : handle -> domid -> unit = "stub_xc_domain_cpuid_apply_policy"
external cpuid_check : handle -> (int64 * (int64 option)) -> string option array -> (bool * string option array) = "stub_xc_cpuid_check"


(** {3 Sizes} *)

external pages_to_kib : int64 -> int64 = "stub_pages_to_kib"
(** [pages_to_kib nb_pages] is the size of [nb_pages] in KiB. *)

val pages_to_mib : int64 -> int64
(** [pages_to_mib nb_pages] is the size of [nb_pages] in MiB. *)

external sizeof_core_header : unit -> int = "stub_sizeof_core_header"
external sizeof_vcpu_guest_context : unit -> int = "stub_sizeof_vcpu_guest_context"
external sizeof_xen_pfn : unit -> int = "stub_sizeof_xen_pfn"

(** {3 Memory barriers} *)

external xen_mb : unit -> unit = "stub_xen_mb" "noalloc"
external xen_rmb : unit -> unit = "stub_xen_rmb" "noalloc"
external xen_wmb : unit -> unit = "stub_xen_wmb" "noalloc"

external hvm_check_pvdriver : handle -> domid -> bool = "stub_xc_hvm_check_pvdriver"

(** {2 Domain debugging functions} *)

type core_magic = Magic_hvm | Magic_pv

type core_header = {
  xch_magic : core_magic;
  xch_nr_vcpus : int;
  xch_nr_pages : nativeint;
  xch_index_offset : int64;
  xch_ctxt_offset : int64;
  xch_pages_offset : int64;
}

external marshall_core_header : core_header -> string = "stub_marshall_core_header"
val coredump : handle -> domid -> Unix.file_descr -> unit
