(*
 * AUTO-GENERATED FILE DO NOT EDIT
 * Generated from xenlight.mli.in and _libxl_types.mli.in
 *)

(*
 * Copyright (C) 2009-2011 Citrix Ltd.
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

type ctx
type domid = int
type devid = int

(* @@LIBXL_TYPES@@ *)
(* AUTO-GENERATED FILE DO NOT EDIT *)
(* autogenerated by 
   genwrap.py /home/jludlam/devel/tmp/myxen/xen-4.9.0/debian/build/build-utils_amd64/tools/ocaml/libs/xl/../../../../tools/libxl/libxl_types.idl _libxl_types.mli.in _libxl_types.ml.in _libxl_types.inc
 *)

(* libxl_error interface *)
type error = 
	 | ERROR_NONSPECIFIC
	 | ERROR_VERSION
	 | ERROR_FAIL
	 | ERROR_NI
	 | ERROR_NOMEM
	 | ERROR_INVAL
	 | ERROR_BADFAIL
	 | ERROR_GUEST_TIMEDOUT
	 | ERROR_TIMEDOUT
	 | ERROR_NOPARAVIRT
	 | ERROR_NOT_READY
	 | ERROR_OSEVENT_REG_FAIL
	 | ERROR_BUFFERFULL
	 | ERROR_UNKNOWN_CHILD
	 | ERROR_LOCK_FAIL
	 | ERROR_JSON_CONFIG_EMPTY
	 | ERROR_DEVICE_EXISTS
	 | ERROR_CHECKPOINT_DEVOPS_DOES_NOT_MATCH
	 | ERROR_CHECKPOINT_DEVICE_NOT_SUPPORTED
	 | ERROR_VNUMA_CONFIG_INVALID
	 | ERROR_DOMAIN_NOTFOUND
	 | ERROR_ABORTED
	 | ERROR_NOTFOUND
	 | ERROR_DOMAIN_DESTROYED
	 | ERROR_FEATURE_REMOVED

val string_of_error : error -> string

(* libxl_domain_type interface *)
type domain_type = 
	 | DOMAIN_TYPE_INVALID
	 | DOMAIN_TYPE_HVM
	 | DOMAIN_TYPE_PV

val string_of_domain_type : domain_type -> string

(* libxl_rdm_reserve_strategy interface *)
type rdm_reserve_strategy = 
	 | RDM_RESERVE_STRATEGY_IGNORE
	 | RDM_RESERVE_STRATEGY_HOST

val string_of_rdm_reserve_strategy : rdm_reserve_strategy -> string

(* libxl_rdm_reserve_policy interface *)
type rdm_reserve_policy = 
	 | RDM_RESERVE_POLICY_INVALID
	 | RDM_RESERVE_POLICY_STRICT
	 | RDM_RESERVE_POLICY_RELAXED

val string_of_rdm_reserve_policy : rdm_reserve_policy -> string

(* libxl_channel_connection interface *)
type channel_connection = 
	 | CHANNEL_CONNECTION_UNKNOWN
	 | CHANNEL_CONNECTION_PTY
	 | CHANNEL_CONNECTION_SOCKET

val string_of_channel_connection : channel_connection -> string

(* libxl_device_model_version interface *)
type device_model_version = 
	 | DEVICE_MODEL_VERSION_UNKNOWN
	 | DEVICE_MODEL_VERSION_QEMU_XEN_TRADITIONAL
	 | DEVICE_MODEL_VERSION_QEMU_XEN
	 | DEVICE_MODEL_VERSION_NONE

val string_of_device_model_version : device_model_version -> string

(* libxl_console_type interface *)
type console_type = 
	 | CONSOLE_TYPE_UNKNOWN
	 | CONSOLE_TYPE_SERIAL
	 | CONSOLE_TYPE_PV

val string_of_console_type : console_type -> string

(* libxl_disk_format interface *)
type disk_format = 
	 | DISK_FORMAT_UNKNOWN
	 | DISK_FORMAT_QCOW
	 | DISK_FORMAT_QCOW2
	 | DISK_FORMAT_VHD
	 | DISK_FORMAT_RAW
	 | DISK_FORMAT_EMPTY
	 | DISK_FORMAT_QED

val string_of_disk_format : disk_format -> string

(* libxl_disk_backend interface *)
type disk_backend = 
	 | DISK_BACKEND_UNKNOWN
	 | DISK_BACKEND_PHY
	 | DISK_BACKEND_TAP
	 | DISK_BACKEND_QDISK

val string_of_disk_backend : disk_backend -> string

(* libxl_nic_type interface *)
type nic_type = 
	 | NIC_TYPE_UNKNOWN
	 | NIC_TYPE_VIF_IOEMU
	 | NIC_TYPE_VIF

val string_of_nic_type : nic_type -> string

(* libxl_action_on_shutdown interface *)
type action_on_shutdown = 
	 | ACTION_ON_SHUTDOWN_DESTROY
	 | ACTION_ON_SHUTDOWN_RESTART
	 | ACTION_ON_SHUTDOWN_RESTART_RENAME
	 | ACTION_ON_SHUTDOWN_PRESERVE
	 | ACTION_ON_SHUTDOWN_COREDUMP_DESTROY
	 | ACTION_ON_SHUTDOWN_COREDUMP_RESTART
	 | ACTION_ON_SHUTDOWN_SOFT_RESET

val string_of_action_on_shutdown : action_on_shutdown -> string

(* libxl_trigger interface *)
type trigger = 
	 | TRIGGER_UNKNOWN
	 | TRIGGER_POWER
	 | TRIGGER_SLEEP
	 | TRIGGER_NMI
	 | TRIGGER_INIT
	 | TRIGGER_RESET
	 | TRIGGER_S3RESUME

val string_of_trigger : trigger -> string

(* libxl_tsc_mode interface *)
type tsc_mode = 
	 | TSC_MODE_DEFAULT
	 | TSC_MODE_ALWAYS_EMULATE
	 | TSC_MODE_NATIVE
	 | TSC_MODE_NATIVE_PARAVIRT

val string_of_tsc_mode : tsc_mode -> string

(* libxl_gfx_passthru_kind interface *)
type gfx_passthru_kind = 
	 | GFX_PASSTHRU_KIND_DEFAULT
	 | GFX_PASSTHRU_KIND_IGD

val string_of_gfx_passthru_kind : gfx_passthru_kind -> string

(* libxl_timer_mode interface *)
type timer_mode = 
	 | TIMER_MODE_UNKNOWN
	 | TIMER_MODE_DELAY_FOR_MISSED_TICKS
	 | TIMER_MODE_NO_DELAY_FOR_MISSED_TICKS
	 | TIMER_MODE_NO_MISSED_TICKS_PENDING
	 | TIMER_MODE_ONE_MISSED_TICK_PENDING

val string_of_timer_mode : timer_mode -> string

(* libxl_bios_type interface *)
type bios_type = 
	 | BIOS_TYPE_UNKNOWN
	 | BIOS_TYPE_ROMBIOS
	 | BIOS_TYPE_SEABIOS
	 | BIOS_TYPE_OVMF

val string_of_bios_type : bios_type -> string

(* libxl_scheduler interface *)
type scheduler = 
	 | SCHEDULER_UNKNOWN
	 | SCHEDULER_SEDF
	 | SCHEDULER_CREDIT
	 | SCHEDULER_CREDIT2
	 | SCHEDULER_ARINC653
	 | SCHEDULER_RTDS
	 | SCHEDULER_NULL

val string_of_scheduler : scheduler -> string

(* libxl_shutdown_reason interface *)
type shutdown_reason = 
	 | SHUTDOWN_REASON_UNKNOWN
	 | SHUTDOWN_REASON_POWEROFF
	 | SHUTDOWN_REASON_REBOOT
	 | SHUTDOWN_REASON_SUSPEND
	 | SHUTDOWN_REASON_CRASH
	 | SHUTDOWN_REASON_WATCHDOG
	 | SHUTDOWN_REASON_SOFT_RESET

val string_of_shutdown_reason : shutdown_reason -> string

(* libxl_vga_interface_type interface *)
type vga_interface_type = 
	 | VGA_INTERFACE_TYPE_UNKNOWN
	 | VGA_INTERFACE_TYPE_CIRRUS
	 | VGA_INTERFACE_TYPE_STD
	 | VGA_INTERFACE_TYPE_NONE
	 | VGA_INTERFACE_TYPE_QXL

val string_of_vga_interface_type : vga_interface_type -> string

(* libxl_vendor_device interface *)
type vendor_device = 
	 | VENDOR_DEVICE_NONE
	 | VENDOR_DEVICE_XENSERVER

val string_of_vendor_device : vendor_device -> string

(* libxl_viridian_enlightenment interface *)
type viridian_enlightenment = 
	 | VIRIDIAN_ENLIGHTENMENT_BASE
	 | VIRIDIAN_ENLIGHTENMENT_FREQ
	 | VIRIDIAN_ENLIGHTENMENT_TIME_REF_COUNT
	 | VIRIDIAN_ENLIGHTENMENT_REFERENCE_TSC
	 | VIRIDIAN_ENLIGHTENMENT_HCALL_REMOTE_TLB_FLUSH
	 | VIRIDIAN_ENLIGHTENMENT_APIC_ASSIST
	 | VIRIDIAN_ENLIGHTENMENT_CRASH_CTL

val string_of_viridian_enlightenment : viridian_enlightenment -> string

(* libxl_hdtype interface *)
type hdtype = 
	 | HDTYPE_IDE
	 | HDTYPE_AHCI

val string_of_hdtype : hdtype -> string

(* libxl_checkpointed_stream interface *)
type checkpointed_stream = 
	 | CHECKPOINTED_STREAM_NONE
	 | CHECKPOINTED_STREAM_REMUS
	 | CHECKPOINTED_STREAM_COLO

val string_of_checkpointed_stream : checkpointed_stream -> string

(* libxl_ioport_range interface *)
module Ioport_range : sig
	type t =
	{
		first : int32;
		number : int32;
	}
	val default : ctx -> unit -> t
end

(* libxl_iomem_range interface *)
module Iomem_range : sig
	type t =
	{
		start : int64;
		number : int64;
		gfn : int64;
	}
	val default : ctx -> unit -> t
end

(* libxl_vga_interface_info interface *)
module Vga_interface_info : sig
	type t =
	{
		kind : vga_interface_type;
	}
	val default : ctx -> unit -> t
end

(* libxl_vnc_info interface *)
module Vnc_info : sig
	type t =
	{
		enable : bool option;
		listen : string option;
		passwd : string option;
		display : int;
		findunused : bool option;
	}
	val default : ctx -> unit -> t
end

(* libxl_spice_info interface *)
module Spice_info : sig
	type t =
	{
		enable : bool option;
		port : int;
		tls_port : int;
		host : string option;
		disable_ticketing : bool option;
		passwd : string option;
		agent_mouse : bool option;
		vdagent : bool option;
		clipboard_sharing : bool option;
		usbredirection : int;
		image_compression : string option;
		streaming_video : string option;
	}
	val default : ctx -> unit -> t
end

(* libxl_sdl_info interface *)
module Sdl_info : sig
	type t =
	{
		enable : bool option;
		opengl : bool option;
		display : string option;
		xauthority : string option;
	}
	val default : ctx -> unit -> t
end

(* libxl_dominfo interface *)
module Dominfo : sig
	type t =
	{
		uuid : int array;
		domid : domid;
		ssidref : int32;
		ssid_label : string option;
		running : bool;
		blocked : bool;
		paused : bool;
		shutdown : bool;
		dying : bool;
		never_stop : bool;
		shutdown_reason : shutdown_reason;
		outstanding_memkb : int64;
		current_memkb : int64;
		shared_memkb : int64;
		paged_memkb : int64;
		max_memkb : int64;
		cpu_time : int64;
		vcpu_max_id : int32;
		vcpu_online : int32;
		cpupool : int32;
		domain_type : domain_type;
	}
	val default : ctx -> unit -> t
	external list : ctx -> t list = "stub_xl_dominfo_list"
	external get : ctx -> domid -> t = "stub_xl_dominfo_get"
end

(* libxl_channelinfo interface *)
module Channelinfo : sig

	type connection_pty =
	{
			path : string option;
	}
	
	type connection__union = Unknown | Pty of connection_pty | Socket
	
	type t =
	{
		backend : string option;
		backend_id : int32;
		frontend : string option;
		frontend_id : int32;
		devid : devid;
		state : int;
		evtch : int;
		rref : int;
		connection : connection__union;
	}
	val default : ctx -> ?connection:channel_connection -> unit -> t
end

(* libxl_vminfo interface *)
module Vminfo : sig
	type t =
	{
		uuid : int array;
		domid : domid;
	}
	val default : ctx -> unit -> t
end

(* libxl_version_info interface *)
module Version_info : sig
	type t =
	{
		xen_version_major : int;
		xen_version_minor : int;
		xen_version_extra : string option;
		compiler : string option;
		compile_by : string option;
		compile_domain : string option;
		compile_date : string option;
		capabilities : string option;
		changeset : string option;
		virt_start : int64;
		pagesize : int;
		commandline : string option;
		build_id : string option;
	}
	val default : ctx -> unit -> t
end

(* libxl_domain_create_info interface *)
module Domain_create_info : sig
	type t =
	{
		xl_type : domain_type;
		hap : bool option;
		oos : bool option;
		ssidref : int32;
		ssid_label : string option;
		name : string option;
		uuid : int array;
		xsdata : (string * string) list;
		platformdata : (string * string) list;
		poolid : int32;
		pool_name : string option;
		run_hotplug_scripts : bool option;
		driver_domain : bool option;
	}
	val default : ctx -> unit -> t
end

(* libxl_domain_restore_params interface *)
module Domain_restore_params : sig
	type t =
	{
		checkpointed_stream : int;
		stream_version : int32;
		colo_proxy_script : string option;
		userspace_colo_proxy : bool option;
	}
	val default : ctx -> unit -> t
end

(* libxl_sched_params interface *)
module Sched_params : sig
	type t =
	{
		vcpuid : int;
		weight : int;
		cap : int;
		period : int;
		extratime : int;
		budget : int;
	}
	val default : ctx -> unit -> t
end

(* libxl_vcpu_sched_params interface *)
module Vcpu_sched_params : sig
	type t =
	{
		sched : scheduler;
		vcpus : Sched_params.t array;
	}
	val default : ctx -> unit -> t
end

(* libxl_domain_sched_params interface *)
module Domain_sched_params : sig
	type t =
	{
		sched : scheduler;
		weight : int;
		cap : int;
		period : int;
		budget : int;
		slice : int;
		latency : int;
		extratime : int;
	}
	val default : ctx -> unit -> t
	external get : ctx -> domid -> t = "stub_xl_domain_sched_params_get"
	external set : ctx -> domid -> t -> unit = "stub_xl_domain_sched_params_set"
end

(* libxl_vnode_info interface *)
module Vnode_info : sig
	type t =
	{
		memkb : int64;
		distances : int32 array;
		pnode : int32;
		vcpus : bool array;
	}
	val default : ctx -> unit -> t
end

(* libxl_gic_version interface *)
type gic_version = 
	 | GIC_VERSION_DEFAULT
	 | GIC_VERSION_V2
	 | GIC_VERSION_V3

val string_of_gic_version : gic_version -> string

(* libxl_rdm_reserve interface *)
module Rdm_reserve : sig
	type t =
	{
		strategy : rdm_reserve_strategy;
		policy : rdm_reserve_policy;
	}
	val default : ctx -> unit -> t
end

(* libxl_altp2m_mode interface *)
type altp2m_mode = 
	 | ALTP2M_MODE_DISABLED
	 | ALTP2M_MODE_MIXED
	 | ALTP2M_MODE_EXTERNAL
	 | ALTP2M_MODE_LIMITED

val string_of_altp2m_mode : altp2m_mode -> string

(* libxl_domain_build_info interface *)
module Domain_build_info : sig

	type type_hvm =
	{
			firmware : string option;
			bios : bios_type;
			pae : bool option;
			apic : bool option;
			acpi : bool option;
			acpi_s3 : bool option;
			acpi_s4 : bool option;
			acpi_laptop_slate : bool option;
			nx : bool option;
			viridian : bool option;
			viridian_enable : bool array;
			viridian_disable : bool array;
			timeoffset : string option;
			hpet : bool option;
			vpt_align : bool option;
			mmio_hole_memkb : int64;
			timer_mode : timer_mode;
			nested_hvm : bool option;
			altp2m : bool option;
			system_firmware : string option;
			smbios_firmware : string option;
			acpi_firmware : string option;
			hdtype : hdtype;
			nographic : bool option;
			vga : Vga_interface_info.t;
			vnc : Vnc_info.t;
			keymap : string option;
			sdl : Sdl_info.t;
			spice : Spice_info.t;
			gfx_passthru : bool option;
			gfx_passthru_kind : gfx_passthru_kind;
			serial : string option;
			boot : string option;
			usb : bool option;
			usbversion : int;
			usbdevice : string option;
			soundhw : string option;
			xen_platform_pci : bool option;
			usbdevice_list : string list;
			vendor_device : vendor_device;
			ms_vm_genid : int array;
			serial_list : string list;
			rdm : Rdm_reserve.t;
			rdm_mem_boundary_memkb : int64;
			mca_caps : int64;
	}
	
	type type_pv =
	{
			kernel : string option;
			slack_memkb : int64;
			bootloader : string option;
			bootloader_args : string list;
			cmdline : string option;
			ramdisk : string option;
			features : string option;
			e820_host : bool option;
	}
	
	type type__union = Hvm of type_hvm | Pv of type_pv | Invalid
	
	type arch_arm__anon = {
		gic_version : gic_version;
	}
	
	type t =
	{
		max_vcpus : int;
		avail_vcpus : bool array;
		cpumap : bool array;
		nodemap : bool array;
		vcpu_hard_affinity : bool array array;
		vcpu_soft_affinity : bool array array;
		numa_placement : bool option;
		tsc_mode : tsc_mode;
		max_memkb : int64;
		target_memkb : int64;
		video_memkb : int64;
		shadow_memkb : int64;
		rtc_timeoffset : int32;
		exec_ssidref : int32;
		exec_ssid_label : string option;
		localtime : bool option;
		disable_migrate : bool option;
		cpuid : unit;
		blkdev_start : string option;
		vnuma_nodes : Vnode_info.t array;
		device_model_version : device_model_version;
		device_model_stubdomain : bool option;
		device_model : string option;
		device_model_ssidref : int32;
		device_model_ssid_label : string option;
		device_model_user : string option;
		extra : string list;
		extra_pv : string list;
		extra_hvm : string list;
		sched_params : Domain_sched_params.t;
		ioports : Ioport_range.t array;
		irqs : int32 array;
		iomem : Iomem_range.t array;
		claim_mode : bool option;
		event_channels : int32;
		kernel : string option;
		cmdline : string option;
		ramdisk : string option;
		device_tree : string option;
		acpi : bool option;
		xl_type : type__union;
		arch_arm : arch_arm__anon;
		altp2m : altp2m_mode;
	}
	val default : ctx -> ?xl_type:domain_type -> unit -> t
end

(* libxl_device_vfb interface *)
module Device_vfb : sig
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		devid : devid;
		vnc : Vnc_info.t;
		sdl : Sdl_info.t;
		keymap : string option;
	}
	val default : ctx -> unit -> t
	external add : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_vfb_add"
	external remove : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_vfb_remove"
	external destroy : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_vfb_destroy"
end

(* libxl_device_vkb interface *)
module Device_vkb : sig
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		devid : devid;
	}
	val default : ctx -> unit -> t
	external add : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_vkb_add"
	external remove : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_vkb_remove"
	external destroy : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_vkb_destroy"
end

(* libxl_device_disk interface *)
module Device_disk : sig
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		pdev_path : string option;
		vdev : string option;
		backend : disk_backend;
		format : disk_format;
		script : string option;
		removable : int;
		readwrite : int;
		is_cdrom : int;
		direct_io_safe : bool;
		discard_enable : bool option;
		colo_enable : bool option;
		colo_restore_enable : bool option;
		colo_host : string option;
		colo_port : int;
		colo_export : string option;
		active_disk : string option;
		hidden_disk : string option;
	}
	val default : ctx -> unit -> t
	external add : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_disk_add"
	external remove : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_disk_remove"
	external destroy : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_disk_destroy"
	external list : ctx -> domid -> t list = "stub_xl_device_disk_list"
	external insert : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_disk_insert"
	external of_vdev : ctx -> domid -> string -> t = "stub_xl_device_disk_of_vdev"
end

(* libxl_device_nic interface *)
module Device_nic : sig
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		devid : devid;
		mtu : int;
		model : string option;
		mac : int array;
		ip : string option;
		bridge : string option;
		ifname : string option;
		script : string option;
		nictype : nic_type;
		rate_bytes_per_interval : int64;
		rate_interval_usecs : int32;
		gatewaydev : string option;
		coloft_forwarddev : string option;
		colo_sock_mirror_id : string option;
		colo_sock_mirror_ip : string option;
		colo_sock_mirror_port : string option;
		colo_sock_compare_pri_in_id : string option;
		colo_sock_compare_pri_in_ip : string option;
		colo_sock_compare_pri_in_port : string option;
		colo_sock_compare_sec_in_id : string option;
		colo_sock_compare_sec_in_ip : string option;
		colo_sock_compare_sec_in_port : string option;
		colo_sock_compare_notify_id : string option;
		colo_sock_compare_notify_ip : string option;
		colo_sock_compare_notify_port : string option;
		colo_sock_redirector0_id : string option;
		colo_sock_redirector0_ip : string option;
		colo_sock_redirector0_port : string option;
		colo_sock_redirector1_id : string option;
		colo_sock_redirector1_ip : string option;
		colo_sock_redirector1_port : string option;
		colo_sock_redirector2_id : string option;
		colo_sock_redirector2_ip : string option;
		colo_sock_redirector2_port : string option;
		colo_filter_mirror_queue : string option;
		colo_filter_mirror_outdev : string option;
		colo_filter_redirector0_queue : string option;
		colo_filter_redirector0_indev : string option;
		colo_filter_redirector0_outdev : string option;
		colo_filter_redirector1_queue : string option;
		colo_filter_redirector1_indev : string option;
		colo_filter_redirector1_outdev : string option;
		colo_compare_pri_in : string option;
		colo_compare_sec_in : string option;
		colo_compare_out : string option;
		colo_compare_notify_dev : string option;
		colo_sock_sec_redirector0_id : string option;
		colo_sock_sec_redirector0_ip : string option;
		colo_sock_sec_redirector0_port : string option;
		colo_sock_sec_redirector1_id : string option;
		colo_sock_sec_redirector1_ip : string option;
		colo_sock_sec_redirector1_port : string option;
		colo_filter_sec_redirector0_queue : string option;
		colo_filter_sec_redirector0_indev : string option;
		colo_filter_sec_redirector0_outdev : string option;
		colo_filter_sec_redirector1_queue : string option;
		colo_filter_sec_redirector1_indev : string option;
		colo_filter_sec_redirector1_outdev : string option;
		colo_filter_sec_rewriter0_queue : string option;
		colo_checkpoint_host : string option;
		colo_checkpoint_port : string option;
	}
	val default : ctx -> unit -> t
	external add : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_nic_add"
	external remove : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_nic_remove"
	external destroy : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_nic_destroy"
	external list : ctx -> domid -> t list = "stub_xl_device_nic_list"
	external of_devid : ctx -> domid -> int -> t = "stub_xl_device_nic_of_devid"
end

(* libxl_device_pci interface *)
module Device_pci : sig
	type t =
	{
		func : int;
		dev : int;
		bus : int;
		domain : int;
		vdevfn : int32;
		vfunc_mask : int32;
		msitranslate : bool;
		power_mgmt : bool;
		permissive : bool;
		seize : bool;
		rdm_policy : rdm_reserve_policy;
	}
	val default : ctx -> unit -> t
	external add : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_pci_add"
	external remove : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_pci_remove"
	external destroy : ctx -> t -> domid -> ?async:'a -> unit -> unit = "stub_xl_device_pci_destroy"
	external list : ctx -> domid -> t list = "stub_xl_device_pci_list"
	external assignable_add : ctx -> t -> bool -> unit = "stub_xl_device_pci_assignable_add"
	external assignable_remove : ctx -> t -> bool -> unit = "stub_xl_device_pci_assignable_remove"
	external assignable_list : ctx -> t list = "stub_xl_device_pci_assignable_list"
end

(* libxl_device_rdm interface *)
module Device_rdm : sig
	type t =
	{
		start : int64;
		size : int64;
		policy : rdm_reserve_policy;
	}
	val default : ctx -> unit -> t
end

(* libxl_usbctrl_type interface *)
type usbctrl_type = 
	 | USBCTRL_TYPE_AUTO
	 | USBCTRL_TYPE_PV
	 | USBCTRL_TYPE_DEVICEMODEL
	 | USBCTRL_TYPE_QUSB

val string_of_usbctrl_type : usbctrl_type -> string

(* libxl_usbdev_type interface *)
type usbdev_type = 
	 | USBDEV_TYPE_HOSTDEV

val string_of_usbdev_type : usbdev_type -> string

(* libxl_device_usbctrl interface *)
module Device_usbctrl : sig
	type t =
	{
		xl_type : usbctrl_type;
		devid : devid;
		version : int;
		ports : int;
		backend_domid : domid;
		backend_domname : string option;
	}
	val default : ctx -> unit -> t
end

(* libxl_device_usbdev interface *)
module Device_usbdev : sig

	type type_hostdev =
	{
			hostbus : int;
			hostaddr : int;
	}
	
	type type__union = Hostdev of type_hostdev
	
	type t =
	{
		ctrl : devid;
		port : int;
		xl_type : type__union;
	}
	val default : ctx -> ?xl_type:usbdev_type -> unit -> t
end

(* libxl_device_dtdev interface *)
module Device_dtdev : sig
	type t =
	{
		path : string option;
	}
	val default : ctx -> unit -> t
end

(* libxl_device_vtpm interface *)
module Device_vtpm : sig
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		devid : devid;
		uuid : int array;
	}
	val default : ctx -> unit -> t
end

(* libxl_device_p9 interface *)
module Device_p9 : sig
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		tag : string option;
		path : string option;
		security_model : string option;
		devid : devid;
	}
	val default : ctx -> unit -> t
end

(* libxl_device_channel interface *)
module Device_channel : sig

	type connection_socket =
	{
			path : string option;
	}
	
	type connection__union = Unknown | Pty | Socket of connection_socket
	
	type t =
	{
		backend_domid : domid;
		backend_domname : string option;
		devid : devid;
		name : string option;
		connection : connection__union;
	}
	val default : ctx -> ?connection:channel_connection -> unit -> t
end

(* libxl_domain_config interface *)
module Domain_config : sig
	type t =
	{
		c_info : Domain_create_info.t;
		b_info : Domain_build_info.t;
		disks : Device_disk.t array;
		nics : Device_nic.t array;
		pcidevs : Device_pci.t array;
		rdms : Device_rdm.t array;
		dtdevs : Device_dtdev.t array;
		vfbs : Device_vfb.t array;
		vkbs : Device_vkb.t array;
		vtpms : Device_vtpm.t array;
		p9 : Device_p9.t array;
		channels : Device_channel.t array;
		usbctrls : Device_usbctrl.t array;
		usbdevs : Device_usbdev.t array;
		on_poweroff : action_on_shutdown;
		on_reboot : action_on_shutdown;
		on_watchdog : action_on_shutdown;
		on_crash : action_on_shutdown;
		on_soft_reset : action_on_shutdown;
	}
	val default : ctx -> unit -> t
end

(* libxl_diskinfo interface *)
module Diskinfo : sig
	type t =
	{
		backend : string option;
		backend_id : int32;
		frontend : string option;
		frontend_id : int32;
		devid : devid;
		state : int;
		evtch : int;
		rref : int;
	}
	val default : ctx -> unit -> t
end

(* libxl_nicinfo interface *)
module Nicinfo : sig
	type t =
	{
		backend : string option;
		backend_id : int32;
		frontend : string option;
		frontend_id : int32;
		devid : devid;
		state : int;
		evtch : int;
		rref_tx : int;
		rref_rx : int;
	}
	val default : ctx -> unit -> t
end

(* libxl_vtpminfo interface *)
module Vtpminfo : sig
	type t =
	{
		backend : string option;
		backend_id : int32;
		frontend : string option;
		frontend_id : int32;
		devid : devid;
		state : int;
		evtch : int;
		rref : int;
		uuid : int array;
	}
	val default : ctx -> unit -> t
end

(* libxl_usbctrlinfo interface *)
module Usbctrlinfo : sig
	type t =
	{
		xl_type : usbctrl_type;
		devid : devid;
		version : int;
		ports : int;
		backend : string option;
		backend_id : int32;
		frontend : string option;
		frontend_id : int32;
		state : int;
		evtch : int;
		ref_urb : int;
		ref_conn : int;
	}
	val default : ctx -> unit -> t
end

(* libxl_physinfo interface *)
module Physinfo : sig
	type t =
	{
		threads_per_core : int32;
		cores_per_socket : int32;
		max_cpu_id : int32;
		nr_cpus : int32;
		cpu_khz : int32;
		total_pages : int64;
		free_pages : int64;
		scrub_pages : int64;
		outstanding_pages : int64;
		sharing_freed_pages : int64;
		sharing_used_frames : int64;
		nr_nodes : int32;
		hw_cap : int32 array;
		cap_hvm : bool;
		cap_hvm_directio : bool;
	}
	val default : ctx -> unit -> t
	external get : ctx -> t = "stub_xl_physinfo_get"
end

(* libxl_numainfo interface *)
module Numainfo : sig
	type t =
	{
		size : int64;
		free : int64;
		dists : int32 array;
	}
	val default : ctx -> unit -> t
end

(* libxl_cputopology interface *)
module Cputopology : sig
	type t =
	{
		core : int32;
		socket : int32;
		node : int32;
	}
	val default : ctx -> unit -> t
	external get : ctx -> t array = "stub_xl_cputopology_get"
end

(* libxl_pcitopology interface *)
module Pcitopology : sig
	type t =
	{
		seg : int;
		bus : int;
		devfn : int;
		node : int32;
	}
	val default : ctx -> unit -> t
end

(* libxl_sched_credit_params interface *)
module Sched_credit_params : sig
	type t =
	{
		tslice_ms : int;
		ratelimit_us : int;
	}
	val default : ctx -> unit -> t
end

(* libxl_sched_credit2_params interface *)
module Sched_credit2_params : sig
	type t =
	{
		ratelimit_us : int;
	}
	val default : ctx -> unit -> t
end

(* libxl_domain_remus_info interface *)
module Domain_remus_info : sig
	type t =
	{
		interval : int;
		allow_unsafe : bool option;
		blackhole : bool option;
		compression : bool option;
		netbuf : bool option;
		netbufscript : string option;
		diskbuf : bool option;
		colo : bool option;
		userspace_colo_proxy : bool option;
	}
	val default : ctx -> unit -> t
end

(* libxl_event_type interface *)
type event_type = 
	 | EVENT_TYPE_DOMAIN_SHUTDOWN
	 | EVENT_TYPE_DOMAIN_DEATH
	 | EVENT_TYPE_DISK_EJECT
	 | EVENT_TYPE_OPERATION_COMPLETE
	 | EVENT_TYPE_DOMAIN_CREATE_CONSOLE_AVAILABLE

val string_of_event_type : event_type -> string

(* libxl_event interface *)
module Event : sig

	type type_domain_shutdown =
	{
			shutdown_reason : int;
	}
	
	type type_disk_eject =
	{
			vdev : string option;
			disk : Device_disk.t;
	}
	
	type type_operation_complete =
	{
			rc : int;
	}
	
	type type__union = Domain_shutdown of type_domain_shutdown | Domain_death | Disk_eject of type_disk_eject | Operation_complete of type_operation_complete | Domain_create_console_available
	
	type t =
	{
		domid : domid;
		domuuid : int array;
		for_user : int64;
		xl_type : type__union;
	}
	val default : ctx -> ?xl_type:event_type -> unit -> t
end

(* libxl_psr_cmt_type interface *)
type psr_cmt_type = 
	 | PSR_CMT_TYPE_CACHE_OCCUPANCY
	 | PSR_CMT_TYPE_TOTAL_MEM_COUNT
	 | PSR_CMT_TYPE_LOCAL_MEM_COUNT

val string_of_psr_cmt_type : psr_cmt_type -> string

(* libxl_psr_cbm_type interface *)
type psr_cbm_type = 
	 | PSR_CBM_TYPE_UNKNOWN
	 | PSR_CBM_TYPE_L3_CBM
	 | PSR_CBM_TYPE_L3_CBM_CODE
	 | PSR_CBM_TYPE_L3_CBM_DATA

val string_of_psr_cbm_type : psr_cbm_type -> string

(* libxl_psr_cat_info interface *)
module Psr_cat_info : sig
	type t =
	{
		id : int32;
		cos_max : int32;
		cbm_len : int32;
		cdp_enabled : bool;
	}
	val default : ctx -> unit -> t
end

(* END OF AUTO-GENERATED CODE *)

exception Error of (error * string)

val register_exceptions: unit -> unit

external ctx_alloc: Xentoollog.handle -> ctx = "stub_libxl_ctx_alloc"

external test_raise_exception: unit -> unit = "stub_raise_exception"

type event =
	| POLLIN (* There is data to read *)
	| POLLPRI (* There is urgent data to read *)
	| POLLOUT (* Writing now will not block *)
	| POLLERR (* Error condition (revents only) *)
	| POLLHUP (* Device has been disconnected (revents only) *)
	| POLLNVAL (* Invalid request: fd not open (revents only). *)

module Domain : sig
	external create_new : ctx -> Domain_config.t -> ?async:'a -> unit -> domid = "stub_libxl_domain_create_new"
	external create_restore : ctx -> Domain_config.t -> (Unix.file_descr * Domain_restore_params.t) ->
		?async:'a -> unit -> domid = "stub_libxl_domain_create_restore"
	external shutdown : ctx -> domid -> unit = "stub_libxl_domain_shutdown"
	external reboot : ctx -> domid -> unit = "stub_libxl_domain_reboot"
	external destroy : ctx -> domid -> ?async:'a -> unit -> unit = "stub_libxl_domain_destroy"
	external suspend : ctx -> domid -> Unix.file_descr -> ?async:'a -> unit -> unit = "stub_libxl_domain_suspend"
	external pause : ctx -> domid -> unit = "stub_libxl_domain_pause"
	external unpause : ctx -> domid -> unit = "stub_libxl_domain_unpause"

	external send_trigger : ctx -> domid -> trigger -> int -> unit = "stub_xl_send_trigger"
	external send_sysrq : ctx -> domid -> char -> unit = "stub_xl_send_sysrq"
end

module Host : sig
	type console_reader
	exception End_of_file

	external xen_console_read_start : ctx -> int -> console_reader  = "stub_libxl_xen_console_read_start"
	external xen_console_read_line : ctx -> console_reader -> string = "stub_libxl_xen_console_read_line"
	external xen_console_read_finish : ctx -> console_reader -> unit = "stub_libxl_xen_console_read_finish"

	external send_debug_keys : ctx -> string -> unit = "stub_xl_send_debug_keys"
end

module Async : sig
	type for_libxl
	type event_hooks
	type osevent_hooks

	val osevent_register_hooks : ctx ->
		user:'a ->
		fd_register:('a -> Unix.file_descr -> event list -> for_libxl -> 'b) ->
		fd_modify:('a -> Unix.file_descr -> 'b -> event list -> 'b) ->
		fd_deregister:('a -> Unix.file_descr -> 'b -> unit) ->
		timeout_register:('a -> int64 -> int64 -> for_libxl -> 'c) ->
		timeout_fire_now:('a -> 'c -> 'c) ->
		osevent_hooks

	external osevent_occurred_fd : ctx -> for_libxl -> Unix.file_descr -> event list -> event list -> unit = "stub_libxl_osevent_occurred_fd"
	external osevent_occurred_timeout : ctx -> for_libxl -> unit = "stub_libxl_osevent_occurred_timeout"

	val async_register_callback :
		async_callback:(result:error option -> user:'a -> unit) ->
		unit

	external evenable_domain_death : ctx -> domid -> int -> unit = "stub_libxl_evenable_domain_death"

	val event_register_callbacks : ctx ->
		user:'a ->
		event_occurs_callback:('a -> Event.t -> unit) ->
		event_disaster_callback:('a -> event_type -> string -> int -> unit) ->
		event_hooks
end
