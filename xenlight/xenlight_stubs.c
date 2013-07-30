/*
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
 */

#include <stdlib.h>

#define CAML_NAME_SPACE
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/signals.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/custom.h>

#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

#include <libxl.h>
#include <libxl_utils.h>

#include <unistd.h>

#include "caml_xentoollog.h"

#define Ctx_val(x)(*((libxl_ctx **) Data_custom_val(x)))
#define CTX ((libxl_ctx *) Ctx_val(ctx))

static char * dup_String_val(value s)
{
	int len;
	char *c;
	len = caml_string_length(s);
	c = calloc(len + 1, sizeof(char));
	if (!c)
		caml_raise_out_of_memory();
	memcpy(c, String_val(s), len);
	return c;
}

static value Val_error(int error)
{
	switch (error) {
	case ERROR_NONSPECIFIC: return Val_int(0);
	case ERROR_VERSION:     return Val_int(1);
	case ERROR_FAIL:        return Val_int(2);
	case ERROR_NI:          return Val_int(3);
	case ERROR_NOMEM:       return Val_int(4);
	case ERROR_INVAL:       return Val_int(5);
	case ERROR_BADFAIL:     return Val_int(6);
	case ERROR_GUEST_TIMEDOUT: return Val_int(7);
	case ERROR_TIMEDOUT:    return Val_int(8);
	case ERROR_NOPARAVIRT:  return Val_int(9);
	case ERROR_NOT_READY:   return Val_int(10);
	case ERROR_OSEVENT_REG_FAIL: return Val_int(11);
	case ERROR_BUFFERFULL:  return Val_int(12);
	case ERROR_UNKNOWN_CHILD: return Val_int(13);
#if 0 /* Let the compiler catch this */
	default:
		caml_raise_sys_error(caml_copy_string("Unknown libxl ERROR"));
		break;
#endif
	}
	/* Should not reach here */
	abort();
}

static void failwith_xl(int error, char *fname)
{
	CAMLlocal1(arg);
	static value *exc = NULL;

	/* First time around, lookup by name */
	if (!exc)
		exc = caml_named_value("Xenlight.Error");

	if (!exc)
		caml_invalid_argument("Exception Xenlight.Error not initialized, please link xenlight.cma");

	arg = caml_alloc(2, 0);

	Store_field(arg, 0, Val_error(error));
	Store_field(arg, 1, caml_copy_string(fname));

	caml_raise_with_arg(*exc, arg);
}

CAMLprim value stub_raise_exception(value unit)
{
	CAMLparam1(unit);
	failwith_xl(ERROR_FAIL, "test exception");
	CAMLreturn(Val_unit);
}

void ctx_finalize(value ctx)
{
	libxl_ctx_free(CTX);
}

static struct custom_operations libxl_ctx_custom_operations = {
	"libxl_ctx_custom_operations",
	ctx_finalize /* custom_finalize_default */,
	custom_compare_default,
	custom_hash_default,
	custom_serialize_default,
	custom_deserialize_default
};

CAMLprim value stub_libxl_ctx_alloc(value logger)
{
	CAMLparam1(logger);
	CAMLlocal1(handle);
	libxl_ctx *ctx;
	int ret;

	ret = libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, (xentoollog_logger *) Xtl_val(logger));
	if (ret != 0) \
		failwith_xl(ERROR_FAIL, "cannot init context");

	handle = caml_alloc_custom(&libxl_ctx_custom_operations, sizeof(ctx), 0, 1);
	Ctx_val(handle) = ctx;

	CAMLreturn(handle);
}

static int list_len(value v)
{
	int len = 0;
	while ( v != Val_emptylist ) {
		len++;
		v = Field(v, 1);
	}
	return len;
}

static int libxl_key_value_list_val(libxl_key_value_list *c_val,
	value v)
{
	CAMLparam1(v);
	CAMLlocal1(elem);
	int nr, i;
	libxl_key_value_list array;

	nr = list_len(v);

	array = calloc((nr + 1) * 2, sizeof(char *));
	if (!array)
		caml_raise_out_of_memory();

	for (i=0; v != Val_emptylist; i++, v = Field(v, 1) ) {
		elem = Field(v, 0);

		array[i * 2] = dup_String_val(Field(elem, 0));
		array[i * 2 + 1] = dup_String_val(Field(elem, 1));
	}

	*c_val = array;
	CAMLreturn(0);
}

static value Val_key_value_list(libxl_key_value_list *c_val)
{
	CAMLparam0();
	CAMLlocal5(list, cons, key, val, kv);
	int i;

	list = Val_emptylist;
	for (i = libxl_string_list_length((libxl_string_list *) c_val) - 1; i >= 0; i -= 2) {
		val = caml_copy_string((char *) c_val[i]);
		key = caml_copy_string((char *) c_val[i - 1]);
		kv = caml_alloc_tuple(2);
		Store_field(kv, 0, key);
		Store_field(kv, 1, val);

		cons = caml_alloc(2, 0);
		Store_field(cons, 0, kv);   // head
		Store_field(cons, 1, list);   // tail
		list = cons;
	}

	CAMLreturn(list);
}

static int libxl_string_list_val(libxl_string_list *c_val, value v)
{
	CAMLparam1(v);
	int nr, i;
	libxl_string_list array;

	nr = list_len(v);

	array = calloc(nr + 1, sizeof(char *));
	if (!array)
		caml_raise_out_of_memory();

	for (i=0; v != Val_emptylist; i++, v = Field(v, 1) )
		array[i] = dup_String_val(Field(v, 0));

	*c_val = array;
	CAMLreturn(0);
}

static value Val_string_list(libxl_string_list *c_val)
{
	CAMLparam0();
	CAMLlocal3(list, cons, string);
	int i;

	list = Val_emptylist;
	for (i = libxl_string_list_length(c_val) - 1; i >= 0; i--) {
		string = caml_copy_string((char *) c_val[i]);
		cons = caml_alloc(2, 0);
		Store_field(cons, 0, string);   // head
		Store_field(cons, 1, list);     // tail
		list = cons;
	}

	CAMLreturn(list);
}

/* Option type support as per http://www.linux-nantes.org/~fmonnier/ocaml/ocaml-wrapping-c.php */
#define Val_none Val_int(0)
#define Some_val(v) Field(v,0)

static value Val_some(value v)
{
	CAMLparam1(v);
	CAMLlocal1(some);
	some = caml_alloc(1, 0);
	Store_field(some, 0, v);
	CAMLreturn(some);
}

static value Val_mac (libxl_mac *c_val)
{
	CAMLparam0();
	CAMLlocal1(v);
	int i;

	v = caml_alloc_tuple(6);

	for(i=0; i<6; i++)
		Store_field(v, i, Val_int((*c_val)[i]));

	CAMLreturn(v);
}

static int Mac_val(libxl_mac *c_val, value v)
{
	CAMLparam1(v);
	int i;

	for(i=0; i<6; i++)
		(*c_val)[i] = Int_val(Field(v, i));

	CAMLreturn(0);
}

static value Val_bitmap (libxl_bitmap *c_val)
{
	CAMLparam0();
	CAMLlocal1(v);
	int i;

	if (c_val->size == 0)
		v = Atom(0);
	else {
	    v = caml_alloc(8 * (c_val->size), 0);
	    libxl_for_each_bit(i, *c_val) {
		    if (libxl_bitmap_test(c_val, i))
			    Store_field(v, i, Val_true);
		    else
			    Store_field(v, i, Val_false);
	    }
	}
	CAMLreturn(v);
}

static int Bitmap_val(libxl_ctx *ctx, libxl_bitmap *c_val, value v)
{
	CAMLparam1(v);
	int i, len = Wosize_val(v);

	c_val->size = 0;
	if (len > 0 && libxl_bitmap_alloc(ctx, c_val, len))
		failwith_xl(ERROR_NOMEM, "cannot allocate bitmap");
	for (i=0; i<len; i++) {
		if (Int_val(Field(v, i)))
			libxl_bitmap_set(c_val, i);
		else
			libxl_bitmap_reset(c_val, i);
	}
	CAMLreturn(0);
}

static value Val_uuid (libxl_uuid *c_val)
{
	CAMLparam0();
	CAMLlocal1(v);
	uint8_t *uuid = libxl_uuid_bytearray(c_val);
	char buf[LIBXL_UUID_FMTLEN+1];

	sprintf(buf, LIBXL_UUID_FMT, LIBXL__UUID_BYTES(uuid));
	v = caml_copy_string(buf);

	CAMLreturn(v);
}

static int Uuid_val(libxl_uuid *c_val, value v)
{
	CAMLparam1(v);
	int i;

	libxl_uuid_from_string(c_val, dup_String_val(v));

	CAMLreturn(0);
}

static value Val_defbool(libxl_defbool c_val)
{
	CAMLparam0();
	CAMLlocal2(v1, v2);
	bool b;

	if (libxl_defbool_is_default(c_val))
		v2 = Val_none;
	else {
		b = libxl_defbool_val(c_val);
		v1 = b ? Val_bool(true) : Val_bool(false);
		v2 = Val_some(v1);
	}
	CAMLreturn(v2);
}

static libxl_defbool Defbool_val(value v)
{
	CAMLparam1(v);
	libxl_defbool db;
	if (v == Val_none)
		libxl_defbool_unset(&db);
	else {
		bool b = Bool_val(Some_val(v));
		libxl_defbool_set(&db, b);
	}
	return db;
}

static value Val_hwcap(libxl_hwcap *c_val)
{
	CAMLparam0();
	CAMLlocal1(hwcap);
	int i;

	hwcap = caml_alloc_tuple(8);
	for (i = 0; i < 8; i++)
		Store_field(hwcap, i, caml_copy_int32((*c_val)[i]));

	CAMLreturn(hwcap);
}

static value Val_string_option(const char *c_val)
{
	CAMLparam0();
	CAMLlocal2(tmp1, tmp2);
	if (c_val) {
		tmp1 = caml_copy_string(c_val);
		tmp2 = Val_some(tmp1);
		CAMLreturn(tmp2);
	}
	else
		CAMLreturn(Val_none);
}

static char *String_option_val(value v)
{
	char *s = NULL;
	if (v != Val_none)
		s = dup_String_val(Some_val(v));
	return s;
}

/* AUTO-GENERATED FILE DO NOT EDIT */
/* autogenerated by 
   genwrap.py /root/xen/tools/ocaml/libs/xl/../../../../tools/libxl/libxl_types.idl _libxl_types.mli.in _libxl_types.ml.in _libxl_types.inc
 */

/* Convert caml value to domain_type */
static int domain_type_val (libxl_ctx *ctx, libxl_domain_type *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_DOMAIN_TYPE_INVALID; break;
	    case 1: *c_val = LIBXL_DOMAIN_TYPE_HVM; break;
	    case 2: *c_val = LIBXL_DOMAIN_TYPE_PV; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_domain_type"); break;
	}
	CAMLreturn(0);
}

/* Convert domain_type to a caml value */
static value Val_domain_type (libxl_domain_type domain_type_c)
{
	CAMLparam0();
	CAMLlocal1(domain_type_ocaml);
	switch(domain_type_c) {
	    case LIBXL_DOMAIN_TYPE_INVALID: domain_type_ocaml = Val_int(0); break;
	    case LIBXL_DOMAIN_TYPE_HVM: domain_type_ocaml = Val_int(1); break;
	    case LIBXL_DOMAIN_TYPE_PV: domain_type_ocaml = Val_int(2); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_domain_type"); break;
	}
	CAMLreturn(domain_type_ocaml);
}

/* Convert caml value to device_model_version */
static int device_model_version_val (libxl_ctx *ctx, libxl_device_model_version *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
#ifdef OCAML_READY
	    case 0: *c_val = LIBXL_DEVICE_MODEL_VERSION_UNKNOWN; break;
#endif
	    case 1: *c_val = LIBXL_DEVICE_MODEL_VERSION_QEMU_XEN_TRADITIONAL; break;
	    case 2: *c_val = LIBXL_DEVICE_MODEL_VERSION_QEMU_XEN; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_device_model_version"); break;
	}
	CAMLreturn(0);
}

/* Convert device_model_version to a caml value */
static value Val_device_model_version (libxl_device_model_version device_model_version_c)
{
	CAMLparam0();
	CAMLlocal1(device_model_version_ocaml);
	switch(device_model_version_c) {
#ifdef OCAML_READY
	    case LIBXL_DEVICE_MODEL_VERSION_UNKNOWN: device_model_version_ocaml = Val_int(0); break;
#endif
	    case LIBXL_DEVICE_MODEL_VERSION_QEMU_XEN_TRADITIONAL: device_model_version_ocaml = Val_int(1); break;
	    case LIBXL_DEVICE_MODEL_VERSION_QEMU_XEN: device_model_version_ocaml = Val_int(2); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_device_model_version"); break;
	}
	CAMLreturn(device_model_version_ocaml);
}

/* Convert caml value to console_type */
static int console_type_val (libxl_ctx *ctx, libxl_console_type *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
#ifdef OCAML_READY
	    case 0: *c_val = LIBXL_CONSOLE_TYPE_UNKNOWN; break;
#endif
	    case 1: *c_val = LIBXL_CONSOLE_TYPE_SERIAL; break;
	    case 2: *c_val = LIBXL_CONSOLE_TYPE_PV; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_console_type"); break;
	}
	CAMLreturn(0);
}

/* Convert console_type to a caml value */
static value Val_console_type (libxl_console_type console_type_c)
{
	CAMLparam0();
	CAMLlocal1(console_type_ocaml);
	switch(console_type_c) {
#ifdef OCAML_READY
	    case LIBXL_CONSOLE_TYPE_UNKNOWN: console_type_ocaml = Val_int(0); break;
#endif
	    case LIBXL_CONSOLE_TYPE_SERIAL: console_type_ocaml = Val_int(1); break;
	    case LIBXL_CONSOLE_TYPE_PV: console_type_ocaml = Val_int(2); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_console_type"); break;
	}
	CAMLreturn(console_type_ocaml);
}

/* Convert caml value to disk_format */
static int disk_format_val (libxl_ctx *ctx, libxl_disk_format *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_DISK_FORMAT_UNKNOWN; break;
	    case 1: *c_val = LIBXL_DISK_FORMAT_QCOW; break;
	    case 2: *c_val = LIBXL_DISK_FORMAT_QCOW2; break;
	    case 3: *c_val = LIBXL_DISK_FORMAT_VHD; break;
	    case 4: *c_val = LIBXL_DISK_FORMAT_RAW; break;
	    case 5: *c_val = LIBXL_DISK_FORMAT_EMPTY; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_disk_format"); break;
	}
	CAMLreturn(0);
}

/* Convert disk_format to a caml value */
static value Val_disk_format (libxl_disk_format disk_format_c)
{
	CAMLparam0();
	CAMLlocal1(disk_format_ocaml);
	switch(disk_format_c) {
	    case LIBXL_DISK_FORMAT_UNKNOWN: disk_format_ocaml = Val_int(0); break;
	    case LIBXL_DISK_FORMAT_QCOW: disk_format_ocaml = Val_int(1); break;
	    case LIBXL_DISK_FORMAT_QCOW2: disk_format_ocaml = Val_int(2); break;
	    case LIBXL_DISK_FORMAT_VHD: disk_format_ocaml = Val_int(3); break;
	    case LIBXL_DISK_FORMAT_RAW: disk_format_ocaml = Val_int(4); break;
	    case LIBXL_DISK_FORMAT_EMPTY: disk_format_ocaml = Val_int(5); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_disk_format"); break;
	}
	CAMLreturn(disk_format_ocaml);
}

/* Convert caml value to disk_backend */
static int disk_backend_val (libxl_ctx *ctx, libxl_disk_backend *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_DISK_BACKEND_UNKNOWN; break;
	    case 1: *c_val = LIBXL_DISK_BACKEND_PHY; break;
	    case 2: *c_val = LIBXL_DISK_BACKEND_TAP; break;
	    case 3: *c_val = LIBXL_DISK_BACKEND_QDISK; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_disk_backend"); break;
	}
	CAMLreturn(0);
}

/* Convert disk_backend to a caml value */
static value Val_disk_backend (libxl_disk_backend disk_backend_c)
{
	CAMLparam0();
	CAMLlocal1(disk_backend_ocaml);
	switch(disk_backend_c) {
	    case LIBXL_DISK_BACKEND_UNKNOWN: disk_backend_ocaml = Val_int(0); break;
	    case LIBXL_DISK_BACKEND_PHY: disk_backend_ocaml = Val_int(1); break;
	    case LIBXL_DISK_BACKEND_TAP: disk_backend_ocaml = Val_int(2); break;
	    case LIBXL_DISK_BACKEND_QDISK: disk_backend_ocaml = Val_int(3); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_disk_backend"); break;
	}
	CAMLreturn(disk_backend_ocaml);
}

/* Convert caml value to nic_type */
static int nic_type_val (libxl_ctx *ctx, libxl_nic_type *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
#ifdef OCAML_READY
	    case 0: *c_val = LIBXL_NIC_TYPE_UNKNOWN; break;
#endif
	    case 1: *c_val = LIBXL_NIC_TYPE_VIF_IOEMU; break;
	    case 2: *c_val = LIBXL_NIC_TYPE_VIF; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_nic_type"); break;
	}
	CAMLreturn(0);
}

/* Convert nic_type to a caml value */
static value Val_nic_type (libxl_nic_type nic_type_c)
{
	CAMLparam0();
	CAMLlocal1(nic_type_ocaml);
	switch(nic_type_c) {
#ifdef OCAML_READY
	    case LIBXL_NIC_TYPE_UNKNOWN: nic_type_ocaml = Val_int(0); break;
#endif
	    case LIBXL_NIC_TYPE_VIF_IOEMU: nic_type_ocaml = Val_int(1); break;
	    case LIBXL_NIC_TYPE_VIF: nic_type_ocaml = Val_int(2); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_nic_type"); break;
	}
	CAMLreturn(nic_type_ocaml);
}

/* Convert caml value to action_on_shutdown */
static int action_on_shutdown_val (libxl_ctx *ctx, libxl_action_on_shutdown *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_ACTION_ON_SHUTDOWN_DESTROY; break;
	    case 1: *c_val = LIBXL_ACTION_ON_SHUTDOWN_RESTART; break;
	    case 2: *c_val = LIBXL_ACTION_ON_SHUTDOWN_RESTART_RENAME; break;
	    case 3: *c_val = LIBXL_ACTION_ON_SHUTDOWN_PRESERVE; break;
	    case 4: *c_val = LIBXL_ACTION_ON_SHUTDOWN_COREDUMP_DESTROY; break;
	    case 5: *c_val = LIBXL_ACTION_ON_SHUTDOWN_COREDUMP_RESTART; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_action_on_shutdown"); break;
	}
	CAMLreturn(0);
}

/* Convert action_on_shutdown to a caml value */
static value Val_action_on_shutdown (libxl_action_on_shutdown action_on_shutdown_c)
{
	CAMLparam0();
	CAMLlocal1(action_on_shutdown_ocaml);
	switch(action_on_shutdown_c) {
	    case LIBXL_ACTION_ON_SHUTDOWN_DESTROY: action_on_shutdown_ocaml = Val_int(0); break;
	    case LIBXL_ACTION_ON_SHUTDOWN_RESTART: action_on_shutdown_ocaml = Val_int(1); break;
	    case LIBXL_ACTION_ON_SHUTDOWN_RESTART_RENAME: action_on_shutdown_ocaml = Val_int(2); break;
	    case LIBXL_ACTION_ON_SHUTDOWN_PRESERVE: action_on_shutdown_ocaml = Val_int(3); break;
	    case LIBXL_ACTION_ON_SHUTDOWN_COREDUMP_DESTROY: action_on_shutdown_ocaml = Val_int(4); break;
	    case LIBXL_ACTION_ON_SHUTDOWN_COREDUMP_RESTART: action_on_shutdown_ocaml = Val_int(5); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_action_on_shutdown"); break;
	}
	CAMLreturn(action_on_shutdown_ocaml);
}

/* Convert caml value to trigger */
static int trigger_val (libxl_ctx *ctx, libxl_trigger *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_TRIGGER_UNKNOWN; break;
	    case 1: *c_val = LIBXL_TRIGGER_POWER; break;
	    case 2: *c_val = LIBXL_TRIGGER_SLEEP; break;
	    case 3: *c_val = LIBXL_TRIGGER_NMI; break;
	    case 4: *c_val = LIBXL_TRIGGER_INIT; break;
	    case 5: *c_val = LIBXL_TRIGGER_RESET; break;
	    case 6: *c_val = LIBXL_TRIGGER_S3RESUME; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_trigger"); break;
	}
	CAMLreturn(0);
}

/* Convert trigger to a caml value */
static value Val_trigger (libxl_trigger trigger_c)
{
	CAMLparam0();
	CAMLlocal1(trigger_ocaml);
	switch(trigger_c) {
	    case LIBXL_TRIGGER_UNKNOWN: trigger_ocaml = Val_int(0); break;
	    case LIBXL_TRIGGER_POWER: trigger_ocaml = Val_int(1); break;
	    case LIBXL_TRIGGER_SLEEP: trigger_ocaml = Val_int(2); break;
	    case LIBXL_TRIGGER_NMI: trigger_ocaml = Val_int(3); break;
	    case LIBXL_TRIGGER_INIT: trigger_ocaml = Val_int(4); break;
	    case LIBXL_TRIGGER_RESET: trigger_ocaml = Val_int(5); break;
	    case LIBXL_TRIGGER_S3RESUME: trigger_ocaml = Val_int(6); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_trigger"); break;
	}
	CAMLreturn(trigger_ocaml);
}

/* Convert caml value to tsc_mode */
static int tsc_mode_val (libxl_ctx *ctx, libxl_tsc_mode *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_TSC_MODE_DEFAULT; break;
	    case 1: *c_val = LIBXL_TSC_MODE_ALWAYS_EMULATE; break;
	    case 2: *c_val = LIBXL_TSC_MODE_NATIVE; break;
	    case 3: *c_val = LIBXL_TSC_MODE_NATIVE_PARAVIRT; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_tsc_mode"); break;
	}
	CAMLreturn(0);
}

/* Convert tsc_mode to a caml value */
static value Val_tsc_mode (libxl_tsc_mode tsc_mode_c)
{
	CAMLparam0();
	CAMLlocal1(tsc_mode_ocaml);
	switch(tsc_mode_c) {
	    case LIBXL_TSC_MODE_DEFAULT: tsc_mode_ocaml = Val_int(0); break;
	    case LIBXL_TSC_MODE_ALWAYS_EMULATE: tsc_mode_ocaml = Val_int(1); break;
	    case LIBXL_TSC_MODE_NATIVE: tsc_mode_ocaml = Val_int(2); break;
	    case LIBXL_TSC_MODE_NATIVE_PARAVIRT: tsc_mode_ocaml = Val_int(3); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_tsc_mode"); break;
	}
	CAMLreturn(tsc_mode_ocaml);
}

/* Convert caml value to timer_mode */
static int timer_mode_val (libxl_ctx *ctx, libxl_timer_mode *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
#ifdef OCAML_READY
	    case 0: *c_val = LIBXL_TIMER_MODE_UNKNOWN; break;
#endif
	    case 1: *c_val = LIBXL_TIMER_MODE_DELAY_FOR_MISSED_TICKS; break;
	    case 2: *c_val = LIBXL_TIMER_MODE_NO_DELAY_FOR_MISSED_TICKS; break;
	    case 3: *c_val = LIBXL_TIMER_MODE_NO_MISSED_TICKS_PENDING; break;
	    case 4: *c_val = LIBXL_TIMER_MODE_ONE_MISSED_TICK_PENDING; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_timer_mode"); break;
	}
	CAMLreturn(0);
}

/* Convert timer_mode to a caml value */
static value Val_timer_mode (libxl_timer_mode timer_mode_c)
{
	CAMLparam0();
	CAMLlocal1(timer_mode_ocaml);
	switch(timer_mode_c) {
#ifdef OCAML_READY
	    case LIBXL_TIMER_MODE_UNKNOWN: timer_mode_ocaml = Val_int(0); break;
#endif
	    case LIBXL_TIMER_MODE_DELAY_FOR_MISSED_TICKS: timer_mode_ocaml = Val_int(1); break;
	    case LIBXL_TIMER_MODE_NO_DELAY_FOR_MISSED_TICKS: timer_mode_ocaml = Val_int(2); break;
	    case LIBXL_TIMER_MODE_NO_MISSED_TICKS_PENDING: timer_mode_ocaml = Val_int(3); break;
	    case LIBXL_TIMER_MODE_ONE_MISSED_TICK_PENDING: timer_mode_ocaml = Val_int(4); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_timer_mode"); break;
	}
	CAMLreturn(timer_mode_ocaml);
}

/* Convert caml value to bios_type */
static int bios_type_val (libxl_ctx *ctx, libxl_bios_type *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
#ifdef OCAML_READY
	    case 0: *c_val = LIBXL_BIOS_TYPE_UNKNOWN; break;
#endif
	    case 1: *c_val = LIBXL_BIOS_TYPE_ROMBIOS; break;
	    case 2: *c_val = LIBXL_BIOS_TYPE_SEABIOS; break;
	    case 3: *c_val = LIBXL_BIOS_TYPE_OVMF; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_bios_type"); break;
	}
	CAMLreturn(0);
}

/* Convert bios_type to a caml value */
static value Val_bios_type (libxl_bios_type bios_type_c)
{
	CAMLparam0();
	CAMLlocal1(bios_type_ocaml);
	switch(bios_type_c) {
#ifdef OCAML_READY
	    case LIBXL_BIOS_TYPE_UNKNOWN: bios_type_ocaml = Val_int(0); break;
#endif
	    case LIBXL_BIOS_TYPE_ROMBIOS: bios_type_ocaml = Val_int(1); break;
	    case LIBXL_BIOS_TYPE_SEABIOS: bios_type_ocaml = Val_int(2); break;
	    case LIBXL_BIOS_TYPE_OVMF: bios_type_ocaml = Val_int(3); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_bios_type"); break;
	}
	CAMLreturn(bios_type_ocaml);
}

/* Convert caml value to scheduler */
static int scheduler_val (libxl_ctx *ctx, libxl_scheduler *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_SCHEDULER_UNKNOWN; break;
	    case 1: *c_val = LIBXL_SCHEDULER_SEDF; break;
	    case 2: *c_val = LIBXL_SCHEDULER_CREDIT; break;
	    case 3: *c_val = LIBXL_SCHEDULER_CREDIT2; break;
	    case 4: *c_val = LIBXL_SCHEDULER_ARINC653; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_scheduler"); break;
	}
	CAMLreturn(0);
}

/* Convert scheduler to a caml value */
static value Val_scheduler (libxl_scheduler scheduler_c)
{
	CAMLparam0();
	CAMLlocal1(scheduler_ocaml);
	switch(scheduler_c) {
	    case LIBXL_SCHEDULER_UNKNOWN: scheduler_ocaml = Val_int(0); break;
	    case LIBXL_SCHEDULER_SEDF: scheduler_ocaml = Val_int(1); break;
	    case LIBXL_SCHEDULER_CREDIT: scheduler_ocaml = Val_int(2); break;
	    case LIBXL_SCHEDULER_CREDIT2: scheduler_ocaml = Val_int(3); break;
	    case LIBXL_SCHEDULER_ARINC653: scheduler_ocaml = Val_int(4); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_scheduler"); break;
	}
	CAMLreturn(scheduler_ocaml);
}

/* Convert caml value to shutdown_reason */
static int shutdown_reason_val (libxl_ctx *ctx, libxl_shutdown_reason *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
#ifdef OCAML_READY
	    case 0: *c_val = LIBXL_SHUTDOWN_REASON_UNKNOWN; break;
#endif
	    case 1: *c_val = LIBXL_SHUTDOWN_REASON_POWEROFF; break;
	    case 2: *c_val = LIBXL_SHUTDOWN_REASON_REBOOT; break;
	    case 3: *c_val = LIBXL_SHUTDOWN_REASON_SUSPEND; break;
	    case 4: *c_val = LIBXL_SHUTDOWN_REASON_CRASH; break;
	    case 5: *c_val = LIBXL_SHUTDOWN_REASON_WATCHDOG; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_shutdown_reason"); break;
	}
	CAMLreturn(0);
}

/* Convert shutdown_reason to a caml value */
static value Val_shutdown_reason (libxl_shutdown_reason shutdown_reason_c)
{
	CAMLparam0();
	CAMLlocal1(shutdown_reason_ocaml);
	switch(shutdown_reason_c) {
#ifdef OCAML_READY
	    case 255:
	    case LIBXL_SHUTDOWN_REASON_UNKNOWN: shutdown_reason_ocaml = Val_int(0); break;
#endif
	    case LIBXL_SHUTDOWN_REASON_POWEROFF: shutdown_reason_ocaml = Val_int(1); break;
	    case LIBXL_SHUTDOWN_REASON_REBOOT: shutdown_reason_ocaml = Val_int(2); break;
	    case LIBXL_SHUTDOWN_REASON_SUSPEND: shutdown_reason_ocaml = Val_int(3); break;
	    case LIBXL_SHUTDOWN_REASON_CRASH: shutdown_reason_ocaml = Val_int(4); break;
	    case LIBXL_SHUTDOWN_REASON_WATCHDOG: shutdown_reason_ocaml = Val_int(5); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_shutdown_reason"); break;
	}
	CAMLreturn(shutdown_reason_ocaml);
}

/* Convert caml value to vga_interface_type */
static int vga_interface_type_val (libxl_ctx *ctx, libxl_vga_interface_type *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_VGA_INTERFACE_TYPE_CIRRUS; break;
	    case 1: *c_val = LIBXL_VGA_INTERFACE_TYPE_STD; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_vga_interface_type"); break;
	}
	CAMLreturn(0);
}

/* Convert vga_interface_type to a caml value */
static value Val_vga_interface_type (libxl_vga_interface_type vga_interface_type_c)
{
	CAMLparam0();
	CAMLlocal1(vga_interface_type_ocaml);
	switch(vga_interface_type_c) {
	    case LIBXL_VGA_INTERFACE_TYPE_CIRRUS: vga_interface_type_ocaml = Val_int(0); break;
	    case LIBXL_VGA_INTERFACE_TYPE_STD: vga_interface_type_ocaml = Val_int(1); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_vga_interface_type"); break;
	}
	CAMLreturn(vga_interface_type_ocaml);
}

/* Convert caml value to ioport_range */
static int ioport_range_val (libxl_ctx *ctx, libxl_ioport_range *c_val, value v)
{
	CAMLparam1(v);

	c_val->first = Int32_val(Field(v, 0));
	c_val->number = Int32_val(Field(v, 1));
	
	CAMLreturn(0);
}

/* Convert ioport_range to a caml value */
static value Val_ioport_range (libxl_ioport_range *ioport_range_c)
{
	CAMLparam0();
	CAMLlocal1(ioport_range_ocaml);
	{
		CAMLlocal1(ioport_range_field);
	
		ioport_range_ocaml = caml_alloc_tuple(2);
	
		ioport_range_field = caml_copy_int32(ioport_range_c->first);
		Store_field(ioport_range_ocaml, 0, ioport_range_field);
	
		ioport_range_field = caml_copy_int32(ioport_range_c->number);
		Store_field(ioport_range_ocaml, 1, ioport_range_field);
	}
	CAMLreturn(ioport_range_ocaml);
}

/* Get the defaults for ioport_range */
value stub_libxl_ioport_range_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_ioport_range c_val;
	libxl_ioport_range_init(&c_val);
	val = Val_ioport_range(&c_val);
	libxl_ioport_range_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to vga_interface_info */
static int vga_interface_info_val (libxl_ctx *ctx, libxl_vga_interface_info *c_val, value v)
{
	CAMLparam1(v);

	vga_interface_type_val(ctx, &c_val->kind, Field(v, 0));
	
	CAMLreturn(0);
}

/* Convert vga_interface_info to a caml value */
static value Val_vga_interface_info (libxl_vga_interface_info *vga_interface_info_c)
{
	CAMLparam0();
	CAMLlocal1(vga_interface_info_ocaml);
	{
		CAMLlocal1(vga_interface_info_field);
	
		vga_interface_info_ocaml = caml_alloc_tuple(1);
	
		vga_interface_info_field = Val_vga_interface_type(vga_interface_info_c->kind);
		Store_field(vga_interface_info_ocaml, 0, vga_interface_info_field);
	}
	CAMLreturn(vga_interface_info_ocaml);
}

/* Get the defaults for vga_interface_info */
value stub_libxl_vga_interface_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_vga_interface_info c_val;
	libxl_vga_interface_info_init(&c_val);
	val = Val_vga_interface_info(&c_val);
	libxl_vga_interface_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to vnc_info */
static int vnc_info_val (libxl_ctx *ctx, libxl_vnc_info *c_val, value v)
{
	CAMLparam1(v);

	c_val->enable = Defbool_val(Field(v, 0));
	c_val->listen = String_option_val(Field(v, 1));
	c_val->passwd = String_option_val(Field(v, 2));
	c_val->display = Int_val(Field(v, 3));
	c_val->findunused = Defbool_val(Field(v, 4));
	
	CAMLreturn(0);
}

/* Convert vnc_info to a caml value */
static value Val_vnc_info (libxl_vnc_info *vnc_info_c)
{
	CAMLparam0();
	CAMLlocal1(vnc_info_ocaml);
	{
		CAMLlocal1(vnc_info_field);
	
		vnc_info_ocaml = caml_alloc_tuple(5);
	
		vnc_info_field = Val_defbool(vnc_info_c->enable);
		Store_field(vnc_info_ocaml, 0, vnc_info_field);
	
		vnc_info_field = Val_string_option(vnc_info_c->listen);
		Store_field(vnc_info_ocaml, 1, vnc_info_field);
	
		vnc_info_field = Val_string_option(vnc_info_c->passwd);
		Store_field(vnc_info_ocaml, 2, vnc_info_field);
	
		vnc_info_field = Val_int(vnc_info_c->display);
		Store_field(vnc_info_ocaml, 3, vnc_info_field);
	
		vnc_info_field = Val_defbool(vnc_info_c->findunused);
		Store_field(vnc_info_ocaml, 4, vnc_info_field);
	}
	CAMLreturn(vnc_info_ocaml);
}

/* Get the defaults for vnc_info */
value stub_libxl_vnc_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_vnc_info c_val;
	libxl_vnc_info_init(&c_val);
	val = Val_vnc_info(&c_val);
	libxl_vnc_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to spice_info */
static int spice_info_val (libxl_ctx *ctx, libxl_spice_info *c_val, value v)
{
	CAMLparam1(v);

	c_val->enable = Defbool_val(Field(v, 0));
	c_val->port = Int_val(Field(v, 1));
	c_val->tls_port = Int_val(Field(v, 2));
	c_val->host = String_option_val(Field(v, 3));
	c_val->disable_ticketing = Defbool_val(Field(v, 4));
	c_val->passwd = String_option_val(Field(v, 5));
	c_val->agent_mouse = Defbool_val(Field(v, 6));
	
	CAMLreturn(0);
}

/* Convert spice_info to a caml value */
static value Val_spice_info (libxl_spice_info *spice_info_c)
{
	CAMLparam0();
	CAMLlocal1(spice_info_ocaml);
	{
		CAMLlocal1(spice_info_field);
	
		spice_info_ocaml = caml_alloc_tuple(7);
	
		spice_info_field = Val_defbool(spice_info_c->enable);
		Store_field(spice_info_ocaml, 0, spice_info_field);
	
		spice_info_field = Val_int(spice_info_c->port);
		Store_field(spice_info_ocaml, 1, spice_info_field);
	
		spice_info_field = Val_int(spice_info_c->tls_port);
		Store_field(spice_info_ocaml, 2, spice_info_field);
	
		spice_info_field = Val_string_option(spice_info_c->host);
		Store_field(spice_info_ocaml, 3, spice_info_field);
	
		spice_info_field = Val_defbool(spice_info_c->disable_ticketing);
		Store_field(spice_info_ocaml, 4, spice_info_field);
	
		spice_info_field = Val_string_option(spice_info_c->passwd);
		Store_field(spice_info_ocaml, 5, spice_info_field);
	
		spice_info_field = Val_defbool(spice_info_c->agent_mouse);
		Store_field(spice_info_ocaml, 6, spice_info_field);
	}
	CAMLreturn(spice_info_ocaml);
}

/* Get the defaults for spice_info */
value stub_libxl_spice_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_spice_info c_val;
	libxl_spice_info_init(&c_val);
	val = Val_spice_info(&c_val);
	libxl_spice_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to sdl_info */
static int sdl_info_val (libxl_ctx *ctx, libxl_sdl_info *c_val, value v)
{
	CAMLparam1(v);

	c_val->enable = Defbool_val(Field(v, 0));
	c_val->opengl = Defbool_val(Field(v, 1));
	c_val->display = String_option_val(Field(v, 2));
	c_val->xauthority = String_option_val(Field(v, 3));
	
	CAMLreturn(0);
}

/* Convert sdl_info to a caml value */
static value Val_sdl_info (libxl_sdl_info *sdl_info_c)
{
	CAMLparam0();
	CAMLlocal1(sdl_info_ocaml);
	{
		CAMLlocal1(sdl_info_field);
	
		sdl_info_ocaml = caml_alloc_tuple(4);
	
		sdl_info_field = Val_defbool(sdl_info_c->enable);
		Store_field(sdl_info_ocaml, 0, sdl_info_field);
	
		sdl_info_field = Val_defbool(sdl_info_c->opengl);
		Store_field(sdl_info_ocaml, 1, sdl_info_field);
	
		sdl_info_field = Val_string_option(sdl_info_c->display);
		Store_field(sdl_info_ocaml, 2, sdl_info_field);
	
		sdl_info_field = Val_string_option(sdl_info_c->xauthority);
		Store_field(sdl_info_ocaml, 3, sdl_info_field);
	}
	CAMLreturn(sdl_info_ocaml);
}

/* Get the defaults for sdl_info */
value stub_libxl_sdl_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_sdl_info c_val;
	libxl_sdl_info_init(&c_val);
	val = Val_sdl_info(&c_val);
	libxl_sdl_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert dominfo to a caml value */
static value Val_dominfo (libxl_dominfo *dominfo_c)
{
	CAMLparam0();
	CAMLlocal1(dominfo_ocaml);
	{
		CAMLlocal1(dominfo_field);
	
		dominfo_ocaml = caml_alloc_tuple(18);
	
		dominfo_field = Val_uuid(&dominfo_c->uuid);
		Store_field(dominfo_ocaml, 0, dominfo_field);
	
		dominfo_field = Val_int(dominfo_c->domid);
		Store_field(dominfo_ocaml, 1, dominfo_field);
	
		dominfo_field = caml_copy_int32(dominfo_c->ssidref);
		Store_field(dominfo_ocaml, 2, dominfo_field);
	
		dominfo_field = Val_bool(dominfo_c->running);
		Store_field(dominfo_ocaml, 3, dominfo_field);
	
		dominfo_field = Val_bool(dominfo_c->blocked);
		Store_field(dominfo_ocaml, 4, dominfo_field);
	
		dominfo_field = Val_bool(dominfo_c->paused);
		Store_field(dominfo_ocaml, 5, dominfo_field);
	
		dominfo_field = Val_bool(dominfo_c->shutdown);
		Store_field(dominfo_ocaml, 6, dominfo_field);
	
		dominfo_field = Val_bool(dominfo_c->dying);
		Store_field(dominfo_ocaml, 7, dominfo_field);
	
		dominfo_field = Val_shutdown_reason(dominfo_c->shutdown_reason);
		Store_field(dominfo_ocaml, 8, dominfo_field);
	
		dominfo_field = caml_copy_int64(dominfo_c->current_memkb);
		Store_field(dominfo_ocaml, 9, dominfo_field);
	
		dominfo_field = caml_copy_int64(dominfo_c->shared_memkb);
		Store_field(dominfo_ocaml, 10, dominfo_field);
	
		dominfo_field = caml_copy_int64(dominfo_c->paged_memkb);
		Store_field(dominfo_ocaml, 11, dominfo_field);
	
		dominfo_field = caml_copy_int64(dominfo_c->max_memkb);
		Store_field(dominfo_ocaml, 12, dominfo_field);
	
		dominfo_field = caml_copy_int64(dominfo_c->cpu_time);
		Store_field(dominfo_ocaml, 13, dominfo_field);
	
		dominfo_field = caml_copy_int32(dominfo_c->vcpu_max_id);
		Store_field(dominfo_ocaml, 14, dominfo_field);
	
		dominfo_field = caml_copy_int32(dominfo_c->vcpu_online);
		Store_field(dominfo_ocaml, 15, dominfo_field);
	
		dominfo_field = caml_copy_int32(dominfo_c->cpupool);
		Store_field(dominfo_ocaml, 16, dominfo_field);
	
#ifdef OCAML_READY
		dominfo_field = Val_domain_type(dominfo_c->domain_type);
#endif
		Store_field(dominfo_ocaml, 17, dominfo_field);
	}
	CAMLreturn(dominfo_ocaml);
}

/* Stubs for dominfo */
value stub_xl_dominfo_list(value v1);
value stub_xl_dominfo_get(value v1, value v2);

/* Get the defaults for dominfo */
value stub_libxl_dominfo_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_dominfo c_val;
	libxl_dominfo_init(&c_val);
	val = Val_dominfo(&c_val);
	libxl_dominfo_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert vminfo to a caml value */
static value Val_vminfo (libxl_vminfo *vminfo_c)
{
	CAMLparam0();
	CAMLlocal1(vminfo_ocaml);
	{
		CAMLlocal1(vminfo_field);
	
		vminfo_ocaml = caml_alloc_tuple(2);
	
		vminfo_field = Val_uuid(&vminfo_c->uuid);
		Store_field(vminfo_ocaml, 0, vminfo_field);
	
		vminfo_field = Val_int(vminfo_c->domid);
		Store_field(vminfo_ocaml, 1, vminfo_field);
	}
	CAMLreturn(vminfo_ocaml);
}

/* Get the defaults for vminfo */
value stub_libxl_vminfo_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_vminfo c_val;
	libxl_vminfo_init(&c_val);
	val = Val_vminfo(&c_val);
	libxl_vminfo_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert version_info to a caml value */
static value Val_version_info (libxl_version_info *version_info_c)
{
	CAMLparam0();
	CAMLlocal1(version_info_ocaml);
	{
		CAMLlocal1(version_info_field);
	
		version_info_ocaml = caml_alloc_tuple(12);
	
		version_info_field = Val_int(version_info_c->xen_version_major);
		Store_field(version_info_ocaml, 0, version_info_field);
	
		version_info_field = Val_int(version_info_c->xen_version_minor);
		Store_field(version_info_ocaml, 1, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->xen_version_extra);
		Store_field(version_info_ocaml, 2, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->compiler);
		Store_field(version_info_ocaml, 3, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->compile_by);
		Store_field(version_info_ocaml, 4, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->compile_domain);
		Store_field(version_info_ocaml, 5, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->compile_date);
		Store_field(version_info_ocaml, 6, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->capabilities);
		Store_field(version_info_ocaml, 7, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->changeset);
		Store_field(version_info_ocaml, 8, version_info_field);
	
		version_info_field = caml_copy_int64(version_info_c->virt_start);
		Store_field(version_info_ocaml, 9, version_info_field);
	
		version_info_field = Val_int(version_info_c->pagesize);
		Store_field(version_info_ocaml, 10, version_info_field);
	
		version_info_field = Val_string_option(version_info_c->commandline);
		Store_field(version_info_ocaml, 11, version_info_field);
	}
	CAMLreturn(version_info_ocaml);
}

/* Get the defaults for version_info */
value stub_libxl_version_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_version_info c_val;
	libxl_version_info_init(&c_val);
	val = Val_version_info(&c_val);
	libxl_version_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to domain_create_info */
static int domain_create_info_val (libxl_ctx *ctx, libxl_domain_create_info *c_val, value v)
{
	CAMLparam1(v);

	domain_type_val(ctx, &c_val->type, Field(v, 0));
	c_val->hap = Defbool_val(Field(v, 1));
	c_val->oos = Defbool_val(Field(v, 2));
	c_val->ssidref = Int32_val(Field(v, 3));
	c_val->name = String_option_val(Field(v, 4));
	Uuid_val(&c_val->uuid, Field(v, 5));
	libxl_key_value_list_val(&c_val->xsdata, Field(v, 6));
	libxl_key_value_list_val(&c_val->platformdata, Field(v, 7));
	c_val->poolid = Int32_val(Field(v, 8));
	c_val->run_hotplug_scripts = Defbool_val(Field(v, 9));
	
	CAMLreturn(0);
}

/* Convert domain_create_info to a caml value */
static value Val_domain_create_info (libxl_domain_create_info *domain_create_info_c)
{
	CAMLparam0();
	CAMLlocal1(domain_create_info_ocaml);
	{
		CAMLlocal1(domain_create_info_field);
	
		domain_create_info_ocaml = caml_alloc_tuple(10);
	
		domain_create_info_field = Val_domain_type(domain_create_info_c->type);
		Store_field(domain_create_info_ocaml, 0, domain_create_info_field);
	
		domain_create_info_field = Val_defbool(domain_create_info_c->hap);
		Store_field(domain_create_info_ocaml, 1, domain_create_info_field);
	
		domain_create_info_field = Val_defbool(domain_create_info_c->oos);
		Store_field(domain_create_info_ocaml, 2, domain_create_info_field);
	
		domain_create_info_field = caml_copy_int32(domain_create_info_c->ssidref);
		Store_field(domain_create_info_ocaml, 3, domain_create_info_field);
	
		domain_create_info_field = Val_string_option(domain_create_info_c->name);
		Store_field(domain_create_info_ocaml, 4, domain_create_info_field);
	
		domain_create_info_field = Val_uuid(&domain_create_info_c->uuid);
		Store_field(domain_create_info_ocaml, 5, domain_create_info_field);
	
		domain_create_info_field = Val_key_value_list(&domain_create_info_c->xsdata);
		Store_field(domain_create_info_ocaml, 6, domain_create_info_field);
	
		domain_create_info_field = Val_key_value_list(&domain_create_info_c->platformdata);
		Store_field(domain_create_info_ocaml, 7, domain_create_info_field);
	
		domain_create_info_field = caml_copy_int32(domain_create_info_c->poolid);
		Store_field(domain_create_info_ocaml, 8, domain_create_info_field);
	
		domain_create_info_field = Val_defbool(domain_create_info_c->run_hotplug_scripts);
		Store_field(domain_create_info_ocaml, 9, domain_create_info_field);
	}
	CAMLreturn(domain_create_info_ocaml);
}

/* Get the defaults for domain_create_info */
value stub_libxl_domain_create_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_domain_create_info c_val;
	libxl_domain_create_info_init(&c_val);
	val = Val_domain_create_info(&c_val);
	libxl_domain_create_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to domain_sched_params */
static int domain_sched_params_val (libxl_ctx *ctx, libxl_domain_sched_params *c_val, value v)
{
	CAMLparam1(v);

	scheduler_val(ctx, &c_val->sched, Field(v, 0));
	c_val->weight = Int_val(Field(v, 1));
	c_val->cap = Int_val(Field(v, 2));
	c_val->period = Int_val(Field(v, 3));
	c_val->slice = Int_val(Field(v, 4));
	c_val->latency = Int_val(Field(v, 5));
	c_val->extratime = Int_val(Field(v, 6));
	
	CAMLreturn(0);
}

/* Convert domain_sched_params to a caml value */
static value Val_domain_sched_params (libxl_domain_sched_params *domain_sched_params_c)
{
	CAMLparam0();
	CAMLlocal1(domain_sched_params_ocaml);
	{
		CAMLlocal1(domain_sched_params_field);
	
		domain_sched_params_ocaml = caml_alloc_tuple(7);
	
		domain_sched_params_field = Val_scheduler(domain_sched_params_c->sched);
		Store_field(domain_sched_params_ocaml, 0, domain_sched_params_field);
	
		domain_sched_params_field = Val_int(domain_sched_params_c->weight);
		Store_field(domain_sched_params_ocaml, 1, domain_sched_params_field);
	
		domain_sched_params_field = Val_int(domain_sched_params_c->cap);
		Store_field(domain_sched_params_ocaml, 2, domain_sched_params_field);
	
		domain_sched_params_field = Val_int(domain_sched_params_c->period);
		Store_field(domain_sched_params_ocaml, 3, domain_sched_params_field);
	
		domain_sched_params_field = Val_int(domain_sched_params_c->slice);
		Store_field(domain_sched_params_ocaml, 4, domain_sched_params_field);
	
		domain_sched_params_field = Val_int(domain_sched_params_c->latency);
		Store_field(domain_sched_params_ocaml, 5, domain_sched_params_field);
	
		domain_sched_params_field = Val_int(domain_sched_params_c->extratime);
		Store_field(domain_sched_params_ocaml, 6, domain_sched_params_field);
	}
	CAMLreturn(domain_sched_params_ocaml);
}

/* Stubs for domain_sched_params */
value stub_xl_domain_sched_params_get(value v1, value v2);
value stub_xl_domain_sched_params_set(value v1, value v2, value v3);

/* Get the defaults for domain_sched_params */
value stub_libxl_domain_sched_params_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_domain_sched_params c_val;
	libxl_domain_sched_params_init(&c_val);
	val = Val_domain_sched_params(&c_val);
	libxl_domain_sched_params_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to domain_build_info */
static int domain_build_info_val (libxl_ctx *ctx, libxl_domain_build_info *c_val, value v)
{
	CAMLparam1(v);

	c_val->max_vcpus = Int_val(Field(v, 0));
	Bitmap_val(ctx, &c_val->avail_vcpus, Field(v, 1));
	Bitmap_val(ctx, &c_val->cpumap, Field(v, 2));
	c_val->numa_placement = Defbool_val(Field(v, 3));
	tsc_mode_val(ctx, &c_val->tsc_mode, Field(v, 4));
	c_val->max_memkb = Int64_val(Field(v, 5));
	c_val->target_memkb = Int64_val(Field(v, 6));
	c_val->video_memkb = Int64_val(Field(v, 7));
	c_val->shadow_memkb = Int64_val(Field(v, 8));
	c_val->rtc_timeoffset = Int32_val(Field(v, 9));
	c_val->localtime = Defbool_val(Field(v, 10));
	c_val->disable_migrate = Defbool_val(Field(v, 11));
	c_val->cpuid = 0;
	c_val->blkdev_start = String_option_val(Field(v, 13));
	device_model_version_val(ctx, &c_val->device_model_version, Field(v, 14));
	c_val->device_model_stubdomain = Defbool_val(Field(v, 15));
	c_val->device_model = String_option_val(Field(v, 16));
	c_val->device_model_ssidref = Int32_val(Field(v, 17));
	libxl_string_list_val(&c_val->extra, Field(v, 18));
	libxl_string_list_val(&c_val->extra_pv, Field(v, 19));
	libxl_string_list_val(&c_val->extra_hvm, Field(v, 20));
	domain_sched_params_val(ctx, &c_val->sched_params, Field(v, 21));
	{
		int i;
		c_val->num_ioports = Wosize_val(Field(v, 22));
		c_val->ioports = (libxl_ioport_range *) calloc(c_val->num_ioports, sizeof(*c_val->ioports));
		for(i=0; i<c_val->num_ioports; i++) {
			ioport_range_val(ctx, &c_val->ioports[i], Field(Field(v, 22), i));
		}
	}
	
	{
		int i;
		c_val->num_irqs = Wosize_val(Field(v, 23));
		c_val->irqs = (uint32_t *) calloc(c_val->num_irqs, sizeof(*c_val->irqs));
		for(i=0; i<c_val->num_irqs; i++) {
			c_val->irqs[i] = Int32_val(Field(Field(v, 23), i));
		}
	}
	
	{
		if(Is_long(Field(v, 24))) {
			switch(Int_val(Field(v, 24))) {
			    case 0: c_val->type = LIBXL_DOMAIN_TYPE_INVALID; break;
			    default: failwith_xl(ERROR_FAIL, "variant handling bug c_val->type (long)"); break;
			}
		} else {
			/* Is block... */
			switch(Tag_val(Field(v, 24))) {
			    case 0:
			        c_val->type = LIBXL_DOMAIN_TYPE_HVM;
			        c_val->u.hvm.firmware = String_option_val(Field(Field(Field(v, 24), 0), 0));
			        bios_type_val(ctx, &c_val->u.hvm.bios, Field(Field(Field(v, 24), 0), 1));
			        c_val->u.hvm.pae = Defbool_val(Field(Field(Field(v, 24), 0), 2));
			        c_val->u.hvm.apic = Defbool_val(Field(Field(Field(v, 24), 0), 3));
			        c_val->u.hvm.acpi = Defbool_val(Field(Field(Field(v, 24), 0), 4));
			        c_val->u.hvm.acpi_s3 = Defbool_val(Field(Field(Field(v, 24), 0), 5));
			        c_val->u.hvm.acpi_s4 = Defbool_val(Field(Field(Field(v, 24), 0), 6));
			        c_val->u.hvm.nx = Defbool_val(Field(Field(Field(v, 24), 0), 7));
			        c_val->u.hvm.viridian = Defbool_val(Field(Field(Field(v, 24), 0), 8));
			        c_val->u.hvm.timeoffset = String_option_val(Field(Field(Field(v, 24), 0), 9));
			        c_val->u.hvm.hpet = Defbool_val(Field(Field(Field(v, 24), 0), 10));
			        c_val->u.hvm.vpt_align = Defbool_val(Field(Field(Field(v, 24), 0), 11));
			        timer_mode_val(ctx, &c_val->u.hvm.timer_mode, Field(Field(Field(v, 24), 0), 12));
			        c_val->u.hvm.nested_hvm = Defbool_val(Field(Field(Field(v, 24), 0), 13));
			        c_val->u.hvm.nographic = Defbool_val(Field(Field(Field(v, 24), 0), 14));
			        vga_interface_info_val(ctx, &c_val->u.hvm.vga, Field(Field(Field(v, 24), 0), 15));
			        vnc_info_val(ctx, &c_val->u.hvm.vnc, Field(Field(Field(v, 24), 0), 16));
			        c_val->u.hvm.keymap = String_option_val(Field(Field(Field(v, 24), 0), 17));
			        sdl_info_val(ctx, &c_val->u.hvm.sdl, Field(Field(Field(v, 24), 0), 18));
			        spice_info_val(ctx, &c_val->u.hvm.spice, Field(Field(Field(v, 24), 0), 19));
			        c_val->u.hvm.gfx_passthru = Defbool_val(Field(Field(Field(v, 24), 0), 20));
			        c_val->u.hvm.serial = String_option_val(Field(Field(Field(v, 24), 0), 21));
			        c_val->u.hvm.boot = String_option_val(Field(Field(Field(v, 24), 0), 22));
			        c_val->u.hvm.usb = Defbool_val(Field(Field(Field(v, 24), 0), 23));
			        c_val->u.hvm.usbdevice = String_option_val(Field(Field(Field(v, 24), 0), 24));
			        c_val->u.hvm.soundhw = String_option_val(Field(Field(Field(v, 24), 0), 25));
			        c_val->u.hvm.xen_platform_pci = Defbool_val(Field(Field(Field(v, 24), 0), 26));
			        break;
			    case 1:
			        c_val->type = LIBXL_DOMAIN_TYPE_PV;
			        c_val->u.pv.kernel = String_option_val(Field(Field(Field(v, 24), 0), 0));
			        c_val->u.pv.slack_memkb = Int64_val(Field(Field(Field(v, 24), 0), 1));
			        c_val->u.pv.bootloader = String_option_val(Field(Field(Field(v, 24), 0), 2));
			        libxl_string_list_val(&c_val->u.pv.bootloader_args, Field(Field(Field(v, 24), 0), 3));
			        c_val->u.pv.cmdline = String_option_val(Field(Field(Field(v, 24), 0), 4));
			        c_val->u.pv.ramdisk = String_option_val(Field(Field(Field(v, 24), 0), 5));
			        c_val->u.pv.features = String_option_val(Field(Field(Field(v, 24), 0), 6));
			        c_val->u.pv.e820_host = Defbool_val(Field(Field(Field(v, 24), 0), 7));
			        break;
			    default: failwith_xl(ERROR_FAIL, "variant handling bug c_val->type (block)"); break;
			}
		}
	}
	
	CAMLreturn(0);
}

/* Convert domain_build_info to a caml value */
static value Val_domain_build_info (libxl_domain_build_info *domain_build_info_c)
{
	CAMLparam0();
	CAMLlocal1(domain_build_info_ocaml);
	{
		CAMLlocal1(domain_build_info_field);
	
		domain_build_info_ocaml = caml_alloc_tuple(25);
	
		domain_build_info_field = Val_int(domain_build_info_c->max_vcpus);
		Store_field(domain_build_info_ocaml, 0, domain_build_info_field);
	
		domain_build_info_field = Val_bitmap(&domain_build_info_c->avail_vcpus);
		Store_field(domain_build_info_ocaml, 1, domain_build_info_field);
	
		domain_build_info_field = Val_bitmap(&domain_build_info_c->cpumap);
		Store_field(domain_build_info_ocaml, 2, domain_build_info_field);
	
		domain_build_info_field = Val_defbool(domain_build_info_c->numa_placement);
		Store_field(domain_build_info_ocaml, 3, domain_build_info_field);
	
		domain_build_info_field = Val_tsc_mode(domain_build_info_c->tsc_mode);
		Store_field(domain_build_info_ocaml, 4, domain_build_info_field);
	
		domain_build_info_field = caml_copy_int64(domain_build_info_c->max_memkb);
		Store_field(domain_build_info_ocaml, 5, domain_build_info_field);
	
		domain_build_info_field = caml_copy_int64(domain_build_info_c->target_memkb);
		Store_field(domain_build_info_ocaml, 6, domain_build_info_field);
	
		domain_build_info_field = caml_copy_int64(domain_build_info_c->video_memkb);
		Store_field(domain_build_info_ocaml, 7, domain_build_info_field);
	
		domain_build_info_field = caml_copy_int64(domain_build_info_c->shadow_memkb);
		Store_field(domain_build_info_ocaml, 8, domain_build_info_field);
	
		domain_build_info_field = caml_copy_int32(domain_build_info_c->rtc_timeoffset);
		Store_field(domain_build_info_ocaml, 9, domain_build_info_field);
	
		domain_build_info_field = Val_defbool(domain_build_info_c->localtime);
		Store_field(domain_build_info_ocaml, 10, domain_build_info_field);
	
		domain_build_info_field = Val_defbool(domain_build_info_c->disable_migrate);
		Store_field(domain_build_info_ocaml, 11, domain_build_info_field);
	
		domain_build_info_field = Val_unit;
		Store_field(domain_build_info_ocaml, 12, domain_build_info_field);
	
		domain_build_info_field = Val_string_option(domain_build_info_c->blkdev_start);
		Store_field(domain_build_info_ocaml, 13, domain_build_info_field);
	
		domain_build_info_field = Val_device_model_version(domain_build_info_c->device_model_version);
		Store_field(domain_build_info_ocaml, 14, domain_build_info_field);
	
		domain_build_info_field = Val_defbool(domain_build_info_c->device_model_stubdomain);
		Store_field(domain_build_info_ocaml, 15, domain_build_info_field);
	
		domain_build_info_field = Val_string_option(domain_build_info_c->device_model);
		Store_field(domain_build_info_ocaml, 16, domain_build_info_field);
	
		domain_build_info_field = caml_copy_int32(domain_build_info_c->device_model_ssidref);
		Store_field(domain_build_info_ocaml, 17, domain_build_info_field);
	
		domain_build_info_field = Val_string_list(&domain_build_info_c->extra);
		Store_field(domain_build_info_ocaml, 18, domain_build_info_field);
	
		domain_build_info_field = Val_string_list(&domain_build_info_c->extra_pv);
		Store_field(domain_build_info_ocaml, 19, domain_build_info_field);
	
		domain_build_info_field = Val_string_list(&domain_build_info_c->extra_hvm);
		Store_field(domain_build_info_ocaml, 20, domain_build_info_field);
	
		domain_build_info_field = Val_domain_sched_params(&domain_build_info_c->sched_params);
		Store_field(domain_build_info_ocaml, 21, domain_build_info_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_build_info_field = caml_alloc(domain_build_info_c->num_ioports,0);
		    for(i=0; i<domain_build_info_c->num_ioports; i++) {
		        array_elem = Val_ioport_range(&domain_build_info_c->ioports[i]);
		        Store_field(domain_build_info_field, i, array_elem);
		    }
		}
		Store_field(domain_build_info_ocaml, 22, domain_build_info_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_build_info_field = caml_alloc(domain_build_info_c->num_irqs,0);
		    for(i=0; i<domain_build_info_c->num_irqs; i++) {
		        array_elem = caml_copy_int32(domain_build_info_c->irqs[i]);
		        Store_field(domain_build_info_field, i, array_elem);
		    }
		}
		Store_field(domain_build_info_ocaml, 23, domain_build_info_field);
	
		switch(domain_build_info_c->type) {
		    case LIBXL_DOMAIN_TYPE_HVM:
		        /* 0: Block */
		        {
			        CAMLlocal1(tmp);
			        domain_build_info_field = caml_alloc(1,0);
			        {
			        	CAMLlocal1(anon_field);
			        
			        	tmp = caml_alloc_tuple(27);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.firmware);
			        	Store_field(tmp, 0, anon_field);
			        
			        	anon_field = Val_bios_type(domain_build_info_c->u.hvm.bios);
			        	Store_field(tmp, 1, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.pae);
			        	Store_field(tmp, 2, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.apic);
			        	Store_field(tmp, 3, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.acpi);
			        	Store_field(tmp, 4, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.acpi_s3);
			        	Store_field(tmp, 5, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.acpi_s4);
			        	Store_field(tmp, 6, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.nx);
			        	Store_field(tmp, 7, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.viridian);
			        	Store_field(tmp, 8, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.timeoffset);
			        	Store_field(tmp, 9, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.hpet);
			        	Store_field(tmp, 10, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.vpt_align);
			        	Store_field(tmp, 11, anon_field);
			        
			        	anon_field = Val_timer_mode(domain_build_info_c->u.hvm.timer_mode);
			        	Store_field(tmp, 12, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.nested_hvm);
			        	Store_field(tmp, 13, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.nographic);
			        	Store_field(tmp, 14, anon_field);
			        
			        	anon_field = Val_vga_interface_info(&domain_build_info_c->u.hvm.vga);
			        	Store_field(tmp, 15, anon_field);
			        
			        	anon_field = Val_vnc_info(&domain_build_info_c->u.hvm.vnc);
			        	Store_field(tmp, 16, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.keymap);
			        	Store_field(tmp, 17, anon_field);
			        
			        	anon_field = Val_sdl_info(&domain_build_info_c->u.hvm.sdl);
			        	Store_field(tmp, 18, anon_field);
			        
			        	anon_field = Val_spice_info(&domain_build_info_c->u.hvm.spice);
			        	Store_field(tmp, 19, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.gfx_passthru);
			        	Store_field(tmp, 20, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.serial);
			        	Store_field(tmp, 21, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.boot);
			        	Store_field(tmp, 22, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.usb);
			        	Store_field(tmp, 23, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.usbdevice);
			        	Store_field(tmp, 24, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.hvm.soundhw);
			        	Store_field(tmp, 25, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.hvm.xen_platform_pci);
			        	Store_field(tmp, 26, anon_field);
			        }
			        Store_field(domain_build_info_field, 0, tmp);
		        }
		        break;
		    case LIBXL_DOMAIN_TYPE_PV:
		        /* 1: Block */
		        {
			        CAMLlocal1(tmp);
			        domain_build_info_field = caml_alloc(1,1);
			        {
			        	CAMLlocal1(anon_field);
			        
			        	tmp = caml_alloc_tuple(8);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.pv.kernel);
			        	Store_field(tmp, 0, anon_field);
			        
			        	anon_field = caml_copy_int64(domain_build_info_c->u.pv.slack_memkb);
			        	Store_field(tmp, 1, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.pv.bootloader);
			        	Store_field(tmp, 2, anon_field);
			        
			        	anon_field = Val_string_list(&domain_build_info_c->u.pv.bootloader_args);
			        	Store_field(tmp, 3, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.pv.cmdline);
			        	Store_field(tmp, 4, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.pv.ramdisk);
			        	Store_field(tmp, 5, anon_field);
			        
			        	anon_field = Val_string_option(domain_build_info_c->u.pv.features);
			        	Store_field(tmp, 6, anon_field);
			        
			        	anon_field = Val_defbool(domain_build_info_c->u.pv.e820_host);
			        	Store_field(tmp, 7, anon_field);
			        }
			        Store_field(domain_build_info_field, 0, tmp);
		        }
		        break;
		    case LIBXL_DOMAIN_TYPE_INVALID:
		        /* 0: None */
		        domain_build_info_field = Val_long(0);
		        break;
		    default: failwith_xl(ERROR_FAIL, "cannot convert value from None"); break;
		}
		Store_field(domain_build_info_ocaml, 24, domain_build_info_field);
	}
	CAMLreturn(domain_build_info_ocaml);
}

/* Get the defaults for domain_build_info */
value stub_libxl_domain_build_info_init(value ctx, value type, value unit)
{
	CAMLparam3(ctx, type, unit);
	CAMLlocal1(val);
	libxl_domain_build_info c_val;
	libxl_domain_build_info_init(&c_val);
	if (type != Val_none) {
		libxl_domain_type c = 0;
		domain_type_val(CTX, &c, Some_val(type));
		libxl_domain_build_info_init_type(&c_val, c);
	}
	val = Val_domain_build_info(&c_val);
	libxl_domain_build_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to device_vfb */
static int device_vfb_val (libxl_ctx *ctx, libxl_device_vfb *c_val, value v)
{
	CAMLparam1(v);

	c_val->backend_domid = Int_val(Field(v, 0));
	c_val->devid = Int_val(Field(v, 1));
	vnc_info_val(ctx, &c_val->vnc, Field(v, 2));
	sdl_info_val(ctx, &c_val->sdl, Field(v, 3));
	c_val->keymap = String_option_val(Field(v, 4));
	
	CAMLreturn(0);
}

/* Convert device_vfb to a caml value */
static value Val_device_vfb (libxl_device_vfb *device_vfb_c)
{
	CAMLparam0();
	CAMLlocal1(device_vfb_ocaml);
	{
		CAMLlocal1(device_vfb_field);
	
		device_vfb_ocaml = caml_alloc_tuple(5);
	
		device_vfb_field = Val_int(device_vfb_c->backend_domid);
		Store_field(device_vfb_ocaml, 0, device_vfb_field);
	
		device_vfb_field = Val_int(device_vfb_c->devid);
		Store_field(device_vfb_ocaml, 1, device_vfb_field);
	
		device_vfb_field = Val_vnc_info(&device_vfb_c->vnc);
		Store_field(device_vfb_ocaml, 2, device_vfb_field);
	
		device_vfb_field = Val_sdl_info(&device_vfb_c->sdl);
		Store_field(device_vfb_ocaml, 3, device_vfb_field);
	
		device_vfb_field = Val_string_option(device_vfb_c->keymap);
		Store_field(device_vfb_ocaml, 4, device_vfb_field);
	}
	CAMLreturn(device_vfb_ocaml);
}

/* Stubs for device_vfb */
value stub_xl_device_vfb_add(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_vfb_remove(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_vfb_destroy(value v1, value v2, value v3, value v4, value v5);

/* Get the defaults for device_vfb */
value stub_libxl_device_vfb_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_device_vfb c_val;
	libxl_device_vfb_init(&c_val);
	val = Val_device_vfb(&c_val);
	libxl_device_vfb_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to device_vkb */
static int device_vkb_val (libxl_ctx *ctx, libxl_device_vkb *c_val, value v)
{
	CAMLparam1(v);

	c_val->backend_domid = Int_val(Field(v, 0));
	c_val->devid = Int_val(Field(v, 1));
	
	CAMLreturn(0);
}

/* Convert device_vkb to a caml value */
static value Val_device_vkb (libxl_device_vkb *device_vkb_c)
{
	CAMLparam0();
	CAMLlocal1(device_vkb_ocaml);
	{
		CAMLlocal1(device_vkb_field);
	
		device_vkb_ocaml = caml_alloc_tuple(2);
	
		device_vkb_field = Val_int(device_vkb_c->backend_domid);
		Store_field(device_vkb_ocaml, 0, device_vkb_field);
	
		device_vkb_field = Val_int(device_vkb_c->devid);
		Store_field(device_vkb_ocaml, 1, device_vkb_field);
	}
	CAMLreturn(device_vkb_ocaml);
}

/* Stubs for device_vkb */
value stub_xl_device_vkb_add(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_vkb_remove(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_vkb_destroy(value v1, value v2, value v3, value v4, value v5);

/* Get the defaults for device_vkb */
value stub_libxl_device_vkb_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_device_vkb c_val;
	libxl_device_vkb_init(&c_val);
	val = Val_device_vkb(&c_val);
	libxl_device_vkb_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to device_disk */
static int device_disk_val (libxl_ctx *ctx, libxl_device_disk *c_val, value v)
{
	CAMLparam1(v);

	c_val->backend_domid = Int_val(Field(v, 0));
	c_val->pdev_path = String_option_val(Field(v, 1));
	c_val->vdev = String_option_val(Field(v, 2));
	disk_backend_val(ctx, &c_val->backend, Field(v, 3));
	disk_format_val(ctx, &c_val->format, Field(v, 4));
	c_val->script = String_option_val(Field(v, 5));
	c_val->removable = Int_val(Field(v, 6));
	c_val->readwrite = Int_val(Field(v, 7));
	c_val->is_cdrom = Int_val(Field(v, 8));
	
	CAMLreturn(0);
}

/* Convert device_disk to a caml value */
static value Val_device_disk (libxl_device_disk *device_disk_c)
{
	CAMLparam0();
	CAMLlocal1(device_disk_ocaml);
	{
		CAMLlocal1(device_disk_field);
	
		device_disk_ocaml = caml_alloc_tuple(9);
	
		device_disk_field = Val_int(device_disk_c->backend_domid);
		Store_field(device_disk_ocaml, 0, device_disk_field);
	
		device_disk_field = Val_string_option(device_disk_c->pdev_path);
		Store_field(device_disk_ocaml, 1, device_disk_field);
	
		device_disk_field = Val_string_option(device_disk_c->vdev);
		Store_field(device_disk_ocaml, 2, device_disk_field);
	
		device_disk_field = Val_disk_backend(device_disk_c->backend);
		Store_field(device_disk_ocaml, 3, device_disk_field);
	
		device_disk_field = Val_disk_format(device_disk_c->format);
		Store_field(device_disk_ocaml, 4, device_disk_field);
	
		device_disk_field = Val_string_option(device_disk_c->script);
		Store_field(device_disk_ocaml, 5, device_disk_field);
	
		device_disk_field = Val_int(device_disk_c->removable);
		Store_field(device_disk_ocaml, 6, device_disk_field);
	
		device_disk_field = Val_int(device_disk_c->readwrite);
		Store_field(device_disk_ocaml, 7, device_disk_field);
	
		device_disk_field = Val_int(device_disk_c->is_cdrom);
		Store_field(device_disk_ocaml, 8, device_disk_field);
	}
	CAMLreturn(device_disk_ocaml);
}

/* Stubs for device_disk */
value stub_xl_device_disk_add(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_disk_remove(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_disk_destroy(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_disk_list(value v1, value v2);
value stub_xl_device_disk_insert(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_disk_of_vdev(value v1, value v2, value v3);

/* Get the defaults for device_disk */
value stub_libxl_device_disk_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_device_disk c_val;
	libxl_device_disk_init(&c_val);
	val = Val_device_disk(&c_val);
	libxl_device_disk_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to device_nic */
static int device_nic_val (libxl_ctx *ctx, libxl_device_nic *c_val, value v)
{
	CAMLparam1(v);

	c_val->backend_domid = Int_val(Field(v, 0));
	c_val->devid = Int_val(Field(v, 1));
	c_val->mtu = Int_val(Field(v, 2));
	c_val->model = String_option_val(Field(v, 3));
	Mac_val(&c_val->mac, Field(v, 4));
	c_val->ip = String_option_val(Field(v, 5));
	c_val->bridge = String_option_val(Field(v, 6));
	c_val->ifname = String_option_val(Field(v, 7));
	c_val->script = String_option_val(Field(v, 8));
	nic_type_val(ctx, &c_val->nictype, Field(v, 9));
	c_val->rate_bytes_per_interval = Int64_val(Field(v, 10));
	c_val->rate_interval_usecs = Int32_val(Field(v, 11));
	
	CAMLreturn(0);
}

/* Convert device_nic to a caml value */
static value Val_device_nic (libxl_device_nic *device_nic_c)
{
	CAMLparam0();
	CAMLlocal1(device_nic_ocaml);
	{
		CAMLlocal1(device_nic_field);
	
		device_nic_ocaml = caml_alloc_tuple(12);
	
		device_nic_field = Val_int(device_nic_c->backend_domid);
		Store_field(device_nic_ocaml, 0, device_nic_field);
	
		device_nic_field = Val_int(device_nic_c->devid);
		Store_field(device_nic_ocaml, 1, device_nic_field);
	
		device_nic_field = Val_int(device_nic_c->mtu);
		Store_field(device_nic_ocaml, 2, device_nic_field);
	
		device_nic_field = Val_string_option(device_nic_c->model);
		Store_field(device_nic_ocaml, 3, device_nic_field);
	
		device_nic_field = Val_mac(&device_nic_c->mac);
		Store_field(device_nic_ocaml, 4, device_nic_field);
	
		device_nic_field = Val_string_option(device_nic_c->ip);
		Store_field(device_nic_ocaml, 5, device_nic_field);
	
		device_nic_field = Val_string_option(device_nic_c->bridge);
		Store_field(device_nic_ocaml, 6, device_nic_field);
	
		device_nic_field = Val_string_option(device_nic_c->ifname);
		Store_field(device_nic_ocaml, 7, device_nic_field);
	
		device_nic_field = Val_string_option(device_nic_c->script);
		Store_field(device_nic_ocaml, 8, device_nic_field);
	
		device_nic_field = Val_nic_type(device_nic_c->nictype);
		Store_field(device_nic_ocaml, 9, device_nic_field);
	
		device_nic_field = caml_copy_int64(device_nic_c->rate_bytes_per_interval);
		Store_field(device_nic_ocaml, 10, device_nic_field);
	
		device_nic_field = caml_copy_int32(device_nic_c->rate_interval_usecs);
		Store_field(device_nic_ocaml, 11, device_nic_field);
	}
	CAMLreturn(device_nic_ocaml);
}

/* Stubs for device_nic */
value stub_xl_device_nic_add(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_nic_remove(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_nic_destroy(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_nic_list(value v1, value v2);
value stub_xl_device_nic_of_devid(value v1, value v2, value v3);

/* Get the defaults for device_nic */
value stub_libxl_device_nic_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_device_nic c_val;
	libxl_device_nic_init(&c_val);
	val = Val_device_nic(&c_val);
	libxl_device_nic_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to device_pci */
static int device_pci_val (libxl_ctx *ctx, libxl_device_pci *c_val, value v)
{
	CAMLparam1(v);

	c_val->func = Int_val(Field(v, 0));
	c_val->dev = Int_val(Field(v, 1));
	c_val->bus = Int_val(Field(v, 2));
	c_val->domain = Int_val(Field(v, 3));
	c_val->vdevfn = Int32_val(Field(v, 4));
	c_val->vfunc_mask = Int32_val(Field(v, 5));
	c_val->msitranslate = Bool_val(Field(v, 6));
	c_val->power_mgmt = Bool_val(Field(v, 7));
	c_val->permissive = Bool_val(Field(v, 8));
	
	CAMLreturn(0);
}

/* Convert device_pci to a caml value */
static value Val_device_pci (libxl_device_pci *device_pci_c)
{
	CAMLparam0();
	CAMLlocal1(device_pci_ocaml);
	{
		CAMLlocal1(device_pci_field);
	
		device_pci_ocaml = caml_alloc_tuple(9);
	
		device_pci_field = Val_int(device_pci_c->func);
		Store_field(device_pci_ocaml, 0, device_pci_field);
	
		device_pci_field = Val_int(device_pci_c->dev);
		Store_field(device_pci_ocaml, 1, device_pci_field);
	
		device_pci_field = Val_int(device_pci_c->bus);
		Store_field(device_pci_ocaml, 2, device_pci_field);
	
		device_pci_field = Val_int(device_pci_c->domain);
		Store_field(device_pci_ocaml, 3, device_pci_field);
	
		device_pci_field = caml_copy_int32(device_pci_c->vdevfn);
		Store_field(device_pci_ocaml, 4, device_pci_field);
	
		device_pci_field = caml_copy_int32(device_pci_c->vfunc_mask);
		Store_field(device_pci_ocaml, 5, device_pci_field);
	
		device_pci_field = Val_bool(device_pci_c->msitranslate);
		Store_field(device_pci_ocaml, 6, device_pci_field);
	
		device_pci_field = Val_bool(device_pci_c->power_mgmt);
		Store_field(device_pci_ocaml, 7, device_pci_field);
	
		device_pci_field = Val_bool(device_pci_c->permissive);
		Store_field(device_pci_ocaml, 8, device_pci_field);
	}
	CAMLreturn(device_pci_ocaml);
}

/* Stubs for device_pci */
value stub_xl_device_pci_add(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_pci_remove(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_pci_destroy(value v1, value v2, value v3, value v4, value v5);
value stub_xl_device_pci_list(value v1, value v2);
value stub_xl_device_pci_assignable_add(value v1, value v2, value v3);
value stub_xl_device_pci_assignable_remove(value v1, value v2, value v3);
value stub_xl_device_pci_assignable_list(value v1);

/* Get the defaults for device_pci */
value stub_libxl_device_pci_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_device_pci c_val;
	libxl_device_pci_init(&c_val);
	val = Val_device_pci(&c_val);
	libxl_device_pci_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to domain_config */
static int domain_config_val (libxl_ctx *ctx, libxl_domain_config *c_val, value v)
{
	CAMLparam1(v);

	domain_create_info_val(ctx, &c_val->c_info, Field(v, 0));
	domain_build_info_val(ctx, &c_val->b_info, Field(v, 1));
	{
		int i;
		c_val->num_disks = Wosize_val(Field(v, 2));
		c_val->disks = (libxl_device_disk *) calloc(c_val->num_disks, sizeof(*c_val->disks));
		for(i=0; i<c_val->num_disks; i++) {
			device_disk_val(ctx, &c_val->disks[i], Field(Field(v, 2), i));
		}
	}
	
	{
		int i;
		c_val->num_nics = Wosize_val(Field(v, 3));
		c_val->nics = (libxl_device_nic *) calloc(c_val->num_nics, sizeof(*c_val->nics));
		for(i=0; i<c_val->num_nics; i++) {
			device_nic_val(ctx, &c_val->nics[i], Field(Field(v, 3), i));
		}
	}
	
	{
		int i;
		c_val->num_pcidevs = Wosize_val(Field(v, 4));
		c_val->pcidevs = (libxl_device_pci *) calloc(c_val->num_pcidevs, sizeof(*c_val->pcidevs));
		for(i=0; i<c_val->num_pcidevs; i++) {
			device_pci_val(ctx, &c_val->pcidevs[i], Field(Field(v, 4), i));
		}
	}
	
	{
		int i;
		c_val->num_vfbs = Wosize_val(Field(v, 5));
		c_val->vfbs = (libxl_device_vfb *) calloc(c_val->num_vfbs, sizeof(*c_val->vfbs));
		for(i=0; i<c_val->num_vfbs; i++) {
			device_vfb_val(ctx, &c_val->vfbs[i], Field(Field(v, 5), i));
		}
	}
	
	{
		int i;
		c_val->num_vkbs = Wosize_val(Field(v, 6));
		c_val->vkbs = (libxl_device_vkb *) calloc(c_val->num_vkbs, sizeof(*c_val->vkbs));
		for(i=0; i<c_val->num_vkbs; i++) {
			device_vkb_val(ctx, &c_val->vkbs[i], Field(Field(v, 6), i));
		}
	}
	
	action_on_shutdown_val(ctx, &c_val->on_poweroff, Field(v, 7));
	action_on_shutdown_val(ctx, &c_val->on_reboot, Field(v, 8));
	action_on_shutdown_val(ctx, &c_val->on_watchdog, Field(v, 9));
	action_on_shutdown_val(ctx, &c_val->on_crash, Field(v, 10));
	
	CAMLreturn(0);
}

/* Convert domain_config to a caml value */
static value Val_domain_config (libxl_domain_config *domain_config_c)
{
	CAMLparam0();
	CAMLlocal1(domain_config_ocaml);
	{
		CAMLlocal1(domain_config_field);
	
		domain_config_ocaml = caml_alloc_tuple(11);
	
		domain_config_field = Val_domain_create_info(&domain_config_c->c_info);
		Store_field(domain_config_ocaml, 0, domain_config_field);
	
		domain_config_field = Val_domain_build_info(&domain_config_c->b_info);
		Store_field(domain_config_ocaml, 1, domain_config_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_config_field = caml_alloc(domain_config_c->num_disks,0);
		    for(i=0; i<domain_config_c->num_disks; i++) {
		        array_elem = Val_device_disk(&domain_config_c->disks[i]);
		        Store_field(domain_config_field, i, array_elem);
		    }
		}
		Store_field(domain_config_ocaml, 2, domain_config_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_config_field = caml_alloc(domain_config_c->num_nics,0);
		    for(i=0; i<domain_config_c->num_nics; i++) {
		        array_elem = Val_device_nic(&domain_config_c->nics[i]);
		        Store_field(domain_config_field, i, array_elem);
		    }
		}
		Store_field(domain_config_ocaml, 3, domain_config_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_config_field = caml_alloc(domain_config_c->num_pcidevs,0);
		    for(i=0; i<domain_config_c->num_pcidevs; i++) {
		        array_elem = Val_device_pci(&domain_config_c->pcidevs[i]);
		        Store_field(domain_config_field, i, array_elem);
		    }
		}
		Store_field(domain_config_ocaml, 4, domain_config_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_config_field = caml_alloc(domain_config_c->num_vfbs,0);
		    for(i=0; i<domain_config_c->num_vfbs; i++) {
		        array_elem = Val_device_vfb(&domain_config_c->vfbs[i]);
		        Store_field(domain_config_field, i, array_elem);
		    }
		}
		Store_field(domain_config_ocaml, 5, domain_config_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    domain_config_field = caml_alloc(domain_config_c->num_vkbs,0);
		    for(i=0; i<domain_config_c->num_vkbs; i++) {
		        array_elem = Val_device_vkb(&domain_config_c->vkbs[i]);
		        Store_field(domain_config_field, i, array_elem);
		    }
		}
		Store_field(domain_config_ocaml, 6, domain_config_field);
	
		domain_config_field = Val_action_on_shutdown(domain_config_c->on_poweroff);
		Store_field(domain_config_ocaml, 7, domain_config_field);
	
		domain_config_field = Val_action_on_shutdown(domain_config_c->on_reboot);
		Store_field(domain_config_ocaml, 8, domain_config_field);
	
		domain_config_field = Val_action_on_shutdown(domain_config_c->on_watchdog);
		Store_field(domain_config_ocaml, 9, domain_config_field);
	
		domain_config_field = Val_action_on_shutdown(domain_config_c->on_crash);
		Store_field(domain_config_ocaml, 10, domain_config_field);
	}
	CAMLreturn(domain_config_ocaml);
}

/* Get the defaults for domain_config */
value stub_libxl_domain_config_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_domain_config c_val;
	libxl_domain_config_init(&c_val);
	val = Val_domain_config(&c_val);
	libxl_domain_config_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert diskinfo to a caml value */
static value Val_diskinfo (libxl_diskinfo *diskinfo_c)
{
	CAMLparam0();
	CAMLlocal1(diskinfo_ocaml);
	{
		CAMLlocal1(diskinfo_field);
	
		diskinfo_ocaml = caml_alloc_tuple(8);
	
		diskinfo_field = Val_string_option(diskinfo_c->backend);
		Store_field(diskinfo_ocaml, 0, diskinfo_field);
	
		diskinfo_field = caml_copy_int32(diskinfo_c->backend_id);
		Store_field(diskinfo_ocaml, 1, diskinfo_field);
	
		diskinfo_field = Val_string_option(diskinfo_c->frontend);
		Store_field(diskinfo_ocaml, 2, diskinfo_field);
	
		diskinfo_field = caml_copy_int32(diskinfo_c->frontend_id);
		Store_field(diskinfo_ocaml, 3, diskinfo_field);
	
		diskinfo_field = Val_int(diskinfo_c->devid);
		Store_field(diskinfo_ocaml, 4, diskinfo_field);
	
		diskinfo_field = Val_int(diskinfo_c->state);
		Store_field(diskinfo_ocaml, 5, diskinfo_field);
	
		diskinfo_field = Val_int(diskinfo_c->evtch);
		Store_field(diskinfo_ocaml, 6, diskinfo_field);
	
		diskinfo_field = Val_int(diskinfo_c->rref);
		Store_field(diskinfo_ocaml, 7, diskinfo_field);
	}
	CAMLreturn(diskinfo_ocaml);
}

/* Get the defaults for diskinfo */
value stub_libxl_diskinfo_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_diskinfo c_val;
	libxl_diskinfo_init(&c_val);
	val = Val_diskinfo(&c_val);
	libxl_diskinfo_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert nicinfo to a caml value */
static value Val_nicinfo (libxl_nicinfo *nicinfo_c)
{
	CAMLparam0();
	CAMLlocal1(nicinfo_ocaml);
	{
		CAMLlocal1(nicinfo_field);
	
		nicinfo_ocaml = caml_alloc_tuple(9);
	
		nicinfo_field = Val_string_option(nicinfo_c->backend);
		Store_field(nicinfo_ocaml, 0, nicinfo_field);
	
		nicinfo_field = caml_copy_int32(nicinfo_c->backend_id);
		Store_field(nicinfo_ocaml, 1, nicinfo_field);
	
		nicinfo_field = Val_string_option(nicinfo_c->frontend);
		Store_field(nicinfo_ocaml, 2, nicinfo_field);
	
		nicinfo_field = caml_copy_int32(nicinfo_c->frontend_id);
		Store_field(nicinfo_ocaml, 3, nicinfo_field);
	
		nicinfo_field = Val_int(nicinfo_c->devid);
		Store_field(nicinfo_ocaml, 4, nicinfo_field);
	
		nicinfo_field = Val_int(nicinfo_c->state);
		Store_field(nicinfo_ocaml, 5, nicinfo_field);
	
		nicinfo_field = Val_int(nicinfo_c->evtch);
		Store_field(nicinfo_ocaml, 6, nicinfo_field);
	
		nicinfo_field = Val_int(nicinfo_c->rref_tx);
		Store_field(nicinfo_ocaml, 7, nicinfo_field);
	
		nicinfo_field = Val_int(nicinfo_c->rref_rx);
		Store_field(nicinfo_ocaml, 8, nicinfo_field);
	}
	CAMLreturn(nicinfo_ocaml);
}

/* Get the defaults for nicinfo */
value stub_libxl_nicinfo_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_nicinfo c_val;
	libxl_nicinfo_init(&c_val);
	val = Val_nicinfo(&c_val);
	libxl_nicinfo_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert physinfo to a caml value */
static value Val_physinfo (libxl_physinfo *physinfo_c)
{
	CAMLparam0();
	CAMLlocal1(physinfo_ocaml);
	{
		CAMLlocal1(physinfo_field);
	
		physinfo_ocaml = caml_alloc_tuple(14);
	
		physinfo_field = caml_copy_int32(physinfo_c->threads_per_core);
		Store_field(physinfo_ocaml, 0, physinfo_field);
	
		physinfo_field = caml_copy_int32(physinfo_c->cores_per_socket);
		Store_field(physinfo_ocaml, 1, physinfo_field);
	
		physinfo_field = caml_copy_int32(physinfo_c->max_cpu_id);
		Store_field(physinfo_ocaml, 2, physinfo_field);
	
		physinfo_field = caml_copy_int32(physinfo_c->nr_cpus);
		Store_field(physinfo_ocaml, 3, physinfo_field);
	
		physinfo_field = caml_copy_int32(physinfo_c->cpu_khz);
		Store_field(physinfo_ocaml, 4, physinfo_field);
	
		physinfo_field = caml_copy_int64(physinfo_c->total_pages);
		Store_field(physinfo_ocaml, 5, physinfo_field);
	
		physinfo_field = caml_copy_int64(physinfo_c->free_pages);
		Store_field(physinfo_ocaml, 6, physinfo_field);
	
		physinfo_field = caml_copy_int64(physinfo_c->scrub_pages);
		Store_field(physinfo_ocaml, 7, physinfo_field);
	
		physinfo_field = caml_copy_int64(physinfo_c->sharing_freed_pages);
		Store_field(physinfo_ocaml, 8, physinfo_field);
	
		physinfo_field = caml_copy_int64(physinfo_c->sharing_used_frames);
		Store_field(physinfo_ocaml, 9, physinfo_field);
	
		physinfo_field = caml_copy_int32(physinfo_c->nr_nodes);
		Store_field(physinfo_ocaml, 10, physinfo_field);
	
		physinfo_field = Val_hwcap(&physinfo_c->hw_cap);
		Store_field(physinfo_ocaml, 11, physinfo_field);
	
		physinfo_field = Val_bool(physinfo_c->cap_hvm);
		Store_field(physinfo_ocaml, 12, physinfo_field);
	
		physinfo_field = Val_bool(physinfo_c->cap_hvm_directio);
		Store_field(physinfo_ocaml, 13, physinfo_field);
	}
	CAMLreturn(physinfo_ocaml);
}

/* Stubs for physinfo */
value stub_xl_physinfo_get(value v1);

/* Get the defaults for physinfo */
value stub_libxl_physinfo_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_physinfo c_val;
	libxl_physinfo_init(&c_val);
	val = Val_physinfo(&c_val);
	libxl_physinfo_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert numainfo to a caml value */
static value Val_numainfo (libxl_numainfo *numainfo_c)
{
	CAMLparam0();
	CAMLlocal1(numainfo_ocaml);
	{
		CAMLlocal1(numainfo_field);
	
		numainfo_ocaml = caml_alloc_tuple(3);
	
		numainfo_field = caml_copy_int64(numainfo_c->size);
		Store_field(numainfo_ocaml, 0, numainfo_field);
	
		numainfo_field = caml_copy_int64(numainfo_c->free);
		Store_field(numainfo_ocaml, 1, numainfo_field);
	
		{
		    int i;
		    CAMLlocal1(array_elem);
		    numainfo_field = caml_alloc(numainfo_c->num_dists,0);
		    for(i=0; i<numainfo_c->num_dists; i++) {
		        array_elem = caml_copy_int32(numainfo_c->dists[i]);
		        Store_field(numainfo_field, i, array_elem);
		    }
		}
		Store_field(numainfo_ocaml, 2, numainfo_field);
	}
	CAMLreturn(numainfo_ocaml);
}

/* Get the defaults for numainfo */
value stub_libxl_numainfo_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_numainfo c_val;
	libxl_numainfo_init(&c_val);
	val = Val_numainfo(&c_val);
	libxl_numainfo_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert cputopology to a caml value */
static value Val_cputopology (libxl_cputopology *cputopology_c)
{
	CAMLparam0();
	CAMLlocal1(cputopology_ocaml);
	{
		CAMLlocal1(cputopology_field);
	
		cputopology_ocaml = caml_alloc_tuple(3);
	
		cputopology_field = caml_copy_int32(cputopology_c->core);
		Store_field(cputopology_ocaml, 0, cputopology_field);
	
		cputopology_field = caml_copy_int32(cputopology_c->socket);
		Store_field(cputopology_ocaml, 1, cputopology_field);
	
		cputopology_field = caml_copy_int32(cputopology_c->node);
		Store_field(cputopology_ocaml, 2, cputopology_field);
	}
	CAMLreturn(cputopology_ocaml);
}

/* Stubs for cputopology */
value stub_xl_cputopology_get(value v1);

/* Get the defaults for cputopology */
value stub_libxl_cputopology_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_cputopology c_val;
	libxl_cputopology_init(&c_val);
	val = Val_cputopology(&c_val);
	libxl_cputopology_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to sched_credit_params */
static int sched_credit_params_val (libxl_ctx *ctx, libxl_sched_credit_params *c_val, value v)
{
	CAMLparam1(v);

	c_val->tslice_ms = Int_val(Field(v, 0));
	c_val->ratelimit_us = Int_val(Field(v, 1));
	
	CAMLreturn(0);
}

/* Convert sched_credit_params to a caml value */
static value Val_sched_credit_params (libxl_sched_credit_params *sched_credit_params_c)
{
	CAMLparam0();
	CAMLlocal1(sched_credit_params_ocaml);
	{
		CAMLlocal1(sched_credit_params_field);
	
		sched_credit_params_ocaml = caml_alloc_tuple(2);
	
		sched_credit_params_field = Val_int(sched_credit_params_c->tslice_ms);
		Store_field(sched_credit_params_ocaml, 0, sched_credit_params_field);
	
		sched_credit_params_field = Val_int(sched_credit_params_c->ratelimit_us);
		Store_field(sched_credit_params_ocaml, 1, sched_credit_params_field);
	}
	CAMLreturn(sched_credit_params_ocaml);
}

/* Get the defaults for sched_credit_params */
value stub_libxl_sched_credit_params_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_sched_credit_params c_val;
	libxl_sched_credit_params_init(&c_val);
	val = Val_sched_credit_params(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to domain_remus_info */
static int domain_remus_info_val (libxl_ctx *ctx, libxl_domain_remus_info *c_val, value v)
{
	CAMLparam1(v);

	c_val->interval = Int_val(Field(v, 0));
	c_val->blackhole = Bool_val(Field(v, 1));
	c_val->compression = Bool_val(Field(v, 2));
	
	CAMLreturn(0);
}

/* Convert domain_remus_info to a caml value */
static value Val_domain_remus_info (libxl_domain_remus_info *domain_remus_info_c)
{
	CAMLparam0();
	CAMLlocal1(domain_remus_info_ocaml);
	{
		CAMLlocal1(domain_remus_info_field);
	
		domain_remus_info_ocaml = caml_alloc_tuple(3);
	
		domain_remus_info_field = Val_int(domain_remus_info_c->interval);
		Store_field(domain_remus_info_ocaml, 0, domain_remus_info_field);
	
		domain_remus_info_field = Val_bool(domain_remus_info_c->blackhole);
		Store_field(domain_remus_info_ocaml, 1, domain_remus_info_field);
	
		domain_remus_info_field = Val_bool(domain_remus_info_c->compression);
		Store_field(domain_remus_info_ocaml, 2, domain_remus_info_field);
	}
	CAMLreturn(domain_remus_info_ocaml);
}

/* Get the defaults for domain_remus_info */
value stub_libxl_domain_remus_info_init(value ctx, value unit)
{
	CAMLparam2(ctx, unit);
	CAMLlocal1(val);
	libxl_domain_remus_info c_val;
	libxl_domain_remus_info_init(&c_val);
	val = Val_domain_remus_info(&c_val);
	libxl_domain_remus_info_dispose(&c_val);
	CAMLreturn(val);
}

/* Convert caml value to event_type */
static int event_type_val (libxl_ctx *ctx, libxl_event_type *c_val, value v)
{
	CAMLparam1(v);

	switch(Int_val(v)) {
	    case 0: *c_val = LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN; break;
	    case 1: *c_val = LIBXL_EVENT_TYPE_DOMAIN_DEATH; break;
	    case 2: *c_val = LIBXL_EVENT_TYPE_DISK_EJECT; break;
	    case 3: *c_val = LIBXL_EVENT_TYPE_OPERATION_COMPLETE; break;
	    case 4: *c_val = LIBXL_EVENT_TYPE_DOMAIN_CREATE_CONSOLE_AVAILABLE; break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value to libxl_event_type"); break;
	}
	CAMLreturn(0);
}

/* Convert event_type to a caml value */
static value Val_event_type (libxl_event_type event_type_c)
{
	CAMLparam0();
	CAMLlocal1(event_type_ocaml);
	switch(event_type_c) {
	    case LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN: event_type_ocaml = Val_int(0); break;
	    case LIBXL_EVENT_TYPE_DOMAIN_DEATH: event_type_ocaml = Val_int(1); break;
	    case LIBXL_EVENT_TYPE_DISK_EJECT: event_type_ocaml = Val_int(2); break;
	    case LIBXL_EVENT_TYPE_OPERATION_COMPLETE: event_type_ocaml = Val_int(3); break;
	    case LIBXL_EVENT_TYPE_DOMAIN_CREATE_CONSOLE_AVAILABLE: event_type_ocaml = Val_int(4); break;
	    default: failwith_xl(ERROR_FAIL, "cannot convert value from libxl_event_type"); break;
	}
	CAMLreturn(event_type_ocaml);
}

/* Convert caml value to event */
static int event_val (libxl_ctx *ctx, libxl_event *c_val, value v)
{
	CAMLparam1(v);

	c_val->domid = Int_val(Field(v, 0));
	Uuid_val(&c_val->domuuid, Field(v, 1));
	c_val->for_user = Int64_val(Field(v, 2));
	{
		if(Is_long(Field(v, 3))) {
			switch(Int_val(Field(v, 3))) {
			    case 0: c_val->type = LIBXL_EVENT_TYPE_DOMAIN_DEATH; break;
			    case 1: c_val->type = LIBXL_EVENT_TYPE_DOMAIN_CREATE_CONSOLE_AVAILABLE; break;
			    default: failwith_xl(ERROR_FAIL, "variant handling bug c_val->type (long)"); break;
			}
		} else {
			/* Is block... */
			switch(Tag_val(Field(v, 3))) {
			    case 0:
			        c_val->type = LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN;
			        c_val->u.domain_shutdown.shutdown_reason = Int_val(Field(Field(Field(v, 3), 0), 0));
			        break;
			    case 1:
			        c_val->type = LIBXL_EVENT_TYPE_DISK_EJECT;
			        c_val->u.disk_eject.vdev = String_option_val(Field(Field(Field(v, 3), 0), 0));
			        device_disk_val(ctx, &c_val->u.disk_eject.disk, Field(Field(Field(v, 3), 0), 1));
			        break;
			    case 2:
			        c_val->type = LIBXL_EVENT_TYPE_OPERATION_COMPLETE;
			        c_val->u.operation_complete.rc = Int_val(Field(Field(Field(v, 3), 0), 0));
			        break;
			    default: failwith_xl(ERROR_FAIL, "variant handling bug c_val->type (block)"); break;
			}
		}
	}
	
	CAMLreturn(0);
}

/* Convert event to a caml value */
static value Val_event (libxl_event *event_c)
{
	CAMLparam0();
	CAMLlocal1(event_ocaml);
	{
		CAMLlocal1(event_field);
	
		event_ocaml = caml_alloc_tuple(5);
	
		event_field = Val_int(event_c->domid);
		Store_field(event_ocaml, 0, event_field);
	
		event_field = Val_uuid(&event_c->domuuid);
		Store_field(event_ocaml, 1, event_field);
	
		event_field = caml_copy_int64(event_c->for_user);
		Store_field(event_ocaml, 2, event_field);
	
		switch(event_c->type) {
		    case LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN:
		        /* 0: Block */
		        {
			        CAMLlocal1(tmp);
			        event_field = caml_alloc(1,0);
			        {
			        	CAMLlocal1(anon_field);
			        
			        	tmp = caml_alloc_tuple(1);
			        
			        	anon_field = Val_int(event_c->u.domain_shutdown.shutdown_reason);
			        	Store_field(tmp, 0, anon_field);
			        }
			        Store_field(event_field, 0, tmp);
		        }
		        break;
		    case LIBXL_EVENT_TYPE_DOMAIN_DEATH:
		        /* 0: Long */
		        event_field = Val_long(0);
		        break;
		    case LIBXL_EVENT_TYPE_DISK_EJECT:
		        /* 1: Block */
		        {
			        CAMLlocal1(tmp);
			        event_field = caml_alloc(1,1);
			        {
			        	CAMLlocal1(anon_field);
			        
			        	tmp = caml_alloc_tuple(2);
			        
			        	anon_field = Val_string_option(event_c->u.disk_eject.vdev);
			        	Store_field(tmp, 0, anon_field);
			        
			        	anon_field = Val_device_disk(&event_c->u.disk_eject.disk);
			        	Store_field(tmp, 1, anon_field);
			        }
			        Store_field(event_field, 0, tmp);
		        }
		        break;
		    case LIBXL_EVENT_TYPE_OPERATION_COMPLETE:
		        /* 2: Block */
		        {
			        CAMLlocal1(tmp);
			        event_field = caml_alloc(1,2);
			        {
			        	CAMLlocal1(anon_field);
			        
			        	tmp = caml_alloc_tuple(1);
			        
			        	anon_field = Val_int(event_c->u.operation_complete.rc);
			        	Store_field(tmp, 0, anon_field);
			        }
			        Store_field(event_field, 0, tmp);
		        }
		        break;
		    case LIBXL_EVENT_TYPE_DOMAIN_CREATE_CONSOLE_AVAILABLE:
		        /* 1: Long */
		        event_field = Val_long(1);
		        break;
		    default: failwith_xl(ERROR_FAIL, "cannot convert value from None"); break;
		}
		Store_field(event_ocaml, 3, event_field);
	}
	CAMLreturn(event_ocaml);
}

/* Get the defaults for event */
value stub_libxl_event_init(value ctx, value type, value unit)
{
	CAMLparam3(ctx, type, unit);
	CAMLlocal1(val);
	libxl_event c_val;
	libxl_event_init(&c_val);
	if (type != Val_none) {
		libxl_event_type c = 0;
		event_type_val(CTX, &c, Some_val(type));
		libxl_event_init_type(&c_val, c);
	}
	val = Val_event(&c_val);
	libxl_event_dispose(&c_val);
	CAMLreturn(val);
}


void async_callback(libxl_ctx *ctx, int rc, void *for_callback)
{
	CAMLparam0();
	CAMLlocal1(error);
	int *task = (int *) for_callback;
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_async_callback");
	}

	if (rc == 0)
		error = Val_none;
	else
		error = Val_some(Val_error(rc));

	caml_callback2(*func, error, (value) for_callback);
}

static int domain_wait_event(libxl_ctx *ctx, int domid, libxl_event **event_r)
{
	int ret;
	for (;;) {
		ret = libxl_event_wait(ctx, event_r, LIBXL_EVENTMASK_ALL, 0,0);
		if (ret) {
			return ret;
		}
		if ((*event_r)->domid != domid) {
			libxl_event_free(CTX, *event_r);
			continue;
		}
		return ret;
	}
}

value stub_libxl_domain_create_new(value ctx, value domain_config, value async, value unit)
{
	CAMLparam4(ctx, async, domain_config, unit);
	int ret;
	libxl_domain_config c_dconfig;
	uint32_t c_domid;
	libxl_asyncop_how ao_how;

	if (async != Val_none) {
		ao_how.callback = async_callback;
		ao_how.u.for_callback = (void *) Some_val(async);
	}

	libxl_domain_config_init(&c_dconfig);
	ret = domain_config_val(CTX, &c_dconfig, domain_config);
	if (ret != 0) {
		libxl_domain_config_dispose(&c_dconfig);
		failwith_xl(ret, "domain_create_new");
	}

	ret = libxl_domain_create_new(CTX, &c_dconfig, &c_domid,
		async != Val_none ? &ao_how : NULL, NULL);

	libxl_domain_config_dispose(&c_dconfig);

	if (ret != 0)
		failwith_xl(ret, "domain_create_new");

	CAMLreturn(Val_int(c_domid));
}

value stub_libxl_domain_create_restore(value ctx, value domain_config, value restore_fd, value async, value unit)
{
	CAMLparam5(ctx, domain_config, restore_fd, async, unit);
	int ret;
	libxl_domain_config c_dconfig;
	uint32_t c_domid;
	libxl_asyncop_how ao_how;

	if (async != Val_none) {
		ao_how.callback = async_callback;
		ao_how.u.for_callback = (void *) Some_val(async);
	}

	libxl_domain_config_init(&c_dconfig);
	ret = domain_config_val(CTX, &c_dconfig, domain_config);
	if (ret != 0) {
		libxl_domain_config_dispose(&c_dconfig);
		failwith_xl(ret, "domain_create_restore");
	}

	ret = libxl_domain_create_restore(CTX, &c_dconfig, &c_domid, Int_val(restore_fd),
		async != Val_none ? &ao_how : NULL, NULL);

	libxl_domain_config_dispose(&c_dconfig);

	if (ret != 0)
		failwith_xl(ret, "domain_create_restore");

	CAMLreturn(Val_int(c_domid));
}

value stub_libxl_domain_wait_shutdown(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;
	libxl_event *event;
	libxl_evgen_domain_death *deathw;
	ret = libxl_evenable_domain_death(CTX, Int_val(domid), 0, &deathw);
	if (ret)
		failwith_xl(ret, "domain_wait_shutdown");

	for (;;) {
		ret = domain_wait_event(CTX, Int_val(domid), &event);
		if (ret) {
			libxl_evdisable_domain_death(CTX, deathw);
			failwith_xl(ret, "domain_wait_shutdown");
		}

		switch (event->type) {
		case LIBXL_EVENT_TYPE_DOMAIN_DEATH:
			goto done;
		case LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN:
			goto done;
		default:
			break;
		}
		libxl_event_free(CTX, event);
	}
done:
	libxl_event_free(CTX, event);
	libxl_evdisable_domain_death(CTX, deathw);

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_shutdown(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_shutdown(CTX, Int_val(domid));
	if (ret != 0)
		failwith_xl(ret, "domain_shutdown");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_reboot(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_reboot(CTX, Int_val(domid));
	if (ret != 0)
		failwith_xl(ret, "domain_reboot");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_destroy(value ctx, value domid, value async, value unit)
{
	CAMLparam4(ctx, domid, async, unit);
	int ret;
	libxl_asyncop_how ao_how;

	if (async != Val_none) {
		ao_how.callback = async_callback;
		ao_how.u.for_callback = (void *) Some_val(async);
	}

	ret = libxl_domain_destroy(CTX, Int_val(domid),
		async != Val_none ? &ao_how : NULL);
	if (ret != 0)
		failwith_xl(ret, "domain_destroy");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_suspend(value ctx, value domid, value fd, value async, value unit)
{
	CAMLparam5(ctx, domid, fd, async, unit);
	int ret;
	libxl_asyncop_how ao_how;

	if (async != Val_none) {
		ao_how.callback = async_callback;
		ao_how.u.for_callback = (void *) Some_val(async);
	}

	ret = libxl_domain_suspend(CTX, Int_val(domid), Int_val(fd), 0,
		async != Val_none ? &ao_how : NULL);
	if (ret != 0)
		failwith_xl(ret, "domain_suspend");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_pause(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_pause(CTX, Int_val(domid));
	if (ret != 0)
		failwith_xl(ret, "domain_pause");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_unpause(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_unpause(CTX, Int_val(domid));
	if (ret != 0)
		failwith_xl(ret, "domain_unpause");

	CAMLreturn(Val_unit);
}

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define _DEVICE_ADDREMOVE(type,fn,op)					\
value stub_xl_device_##type##_##op(value ctx, value info, value domid,	\
	value async, value unit)					\
{									\
	CAMLparam5(ctx, info, domid, async, unit);			\
	libxl_device_##type c_info;					\
	int ret, marker_var;						\
	libxl_asyncop_how ao_how;					\
									\
	device_##type##_val(CTX, &c_info, info);			\
									\
	if (async != Val_none) {					\
		ao_how.callback = async_callback;			\
		ao_how.u.for_callback = (void *) Some_val(async);	\
	}								\
									\
	ret = libxl_##fn##_##op(CTX, Int_val(domid), &c_info,		\
		async != Val_none ? &ao_how : NULL);			\
									\
	libxl_device_##type##_dispose(&c_info);				\
									\
	if (ret != 0)							\
		failwith_xl(ret, STRINGIFY(type) "_" STRINGIFY(op));	\
									\
	CAMLreturn(Val_unit);						\
}

#define DEVICE_ADDREMOVE(type) \
	_DEVICE_ADDREMOVE(type, device_##type, add) \
 	_DEVICE_ADDREMOVE(type, device_##type, remove) \
 	_DEVICE_ADDREMOVE(type, device_##type, destroy)

DEVICE_ADDREMOVE(disk)
DEVICE_ADDREMOVE(nic)
DEVICE_ADDREMOVE(vfb)
DEVICE_ADDREMOVE(vkb)
DEVICE_ADDREMOVE(pci)
_DEVICE_ADDREMOVE(disk, cdrom, insert)

value stub_xl_device_nic_of_devid(value ctx, value domid, value devid)
{
	CAMLparam3(ctx, domid, devid);
	libxl_device_nic nic;
	libxl_devid_to_device_nic(CTX, Int_val(domid), Int_val(devid), &nic);
	CAMLreturn(Val_device_nic(&nic));
}

value stub_xl_device_nic_list(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal2(list, temp);
	libxl_device_nic *c_list;
	int i, nb;
	uint32_t c_domid;

	c_domid = Int_val(domid);

	c_list = libxl_device_nic_list(CTX, c_domid, &nb);
	if (!c_list)
		failwith_xl(ERROR_FAIL, "nic_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_nic(&c_list[i]));
		libxl_device_nic_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_device_disk_list(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal2(list, temp);
	libxl_device_disk *c_list;
	int i, nb;
	uint32_t c_domid;

	c_domid = Int_val(domid);

	c_list = libxl_device_disk_list(CTX, c_domid, &nb);
	if (!c_list)
		failwith_xl(ERROR_FAIL, "disk_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_disk(&c_list[i]));
		libxl_device_disk_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_device_disk_of_vdev(value ctx, value domid, value vdev)
{
	CAMLparam3(ctx, domid, vdev);
	libxl_device_disk disk;
	libxl_vdev_to_device_disk(CTX, Int_val(domid), String_val(vdev), &disk);
	CAMLreturn(Val_device_disk(&disk));
}

value stub_xl_device_pci_list(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal2(list, temp);
	libxl_device_pci *c_list;
	int i, nb;
	uint32_t c_domid;

	c_domid = Int_val(domid);

	c_list = libxl_device_pci_list(CTX, c_domid, &nb);
	if (!c_list)
		failwith_xl(ERROR_FAIL, "pci_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_pci(&c_list[i]));
		libxl_device_pci_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_device_pci_assignable_add(value ctx, value info, value rebind)
{
	CAMLparam3(ctx, info, rebind);
	libxl_device_pci c_info;
	int ret, marker_var;

	device_pci_val(CTX, &c_info, info);

	ret = libxl_device_pci_assignable_add(CTX, &c_info, (int) Bool_val(rebind));

	libxl_device_pci_dispose(&c_info);

	if (ret != 0)
		failwith_xl(ret, "pci_assignable_add");

	CAMLreturn(Val_unit);
}

value stub_xl_device_pci_assignable_remove(value ctx, value info, value rebind)
{
	CAMLparam3(ctx, info, rebind);
	libxl_device_pci c_info;
	int ret, marker_var;

	device_pci_val(CTX, &c_info, info);

	ret = libxl_device_pci_assignable_remove(CTX, &c_info, (int) Bool_val(rebind));

	libxl_device_pci_dispose(&c_info);

	if (ret != 0)
		failwith_xl(ret, "pci_assignable_remove");

	CAMLreturn(Val_unit);
}

value stub_xl_device_pci_assignable_list(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal2(list, temp);
	libxl_device_pci *c_list;
	int i, nb;
	uint32_t c_domid;

	c_list = libxl_device_pci_assignable_list(CTX, &nb);
	if (!c_list)
		failwith_xl(ERROR_FAIL, "pci_assignable_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_pci(&c_list[i]));
		libxl_device_pci_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_physinfo_get(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal1(physinfo);
	libxl_physinfo c_physinfo;
	int ret;

	ret = libxl_get_physinfo(CTX, &c_physinfo);

	if (ret != 0)
		failwith_xl(ret, "get_physinfo");

	physinfo = Val_physinfo(&c_physinfo);

	libxl_physinfo_dispose(&c_physinfo);

	CAMLreturn(physinfo);
}

value stub_xl_cputopology_get(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal3(topology, v, v0);
	libxl_cputopology *c_topology;
	int i, nr;

	c_topology = libxl_get_cpu_topology(CTX, &nr);

	if (!c_topology)
		failwith_xl(ERROR_FAIL, "get_cpu_topologyinfo");

	topology = caml_alloc_tuple(nr);
	for (i = 0; i < nr; i++) {
		if (c_topology[i].core != LIBXL_CPUTOPOLOGY_INVALID_ENTRY) {
			v0 = Val_cputopology(&c_topology[i]);
			v = Val_some(v0);
		}
		else
			v = Val_none;
		Store_field(topology, i, v);
	}

	libxl_cputopology_list_free(c_topology, nr);

	CAMLreturn(topology);
}

value stub_xl_dominfo_list(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal2(domlist, temp);
	libxl_dominfo *c_domlist;
	int i, nb;

	c_domlist = libxl_list_domain(CTX, &nb);
	if (!c_domlist)
		failwith_xl(ERROR_FAIL, "dominfo_list");

	domlist = temp = Val_emptylist;
	for (i = nb - 1; i >= 0; i--) {
		domlist = caml_alloc_small(2, Tag_cons);
		Field(domlist, 0) = Val_int(0);
		Field(domlist, 1) = temp;
		temp = domlist;

		Store_field(domlist, 0, Val_dominfo(&c_domlist[i]));
	}

	libxl_dominfo_list_free(c_domlist, nb);

	CAMLreturn(domlist);
}

value stub_xl_dominfo_get(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal1(dominfo);
	libxl_dominfo c_dominfo;
	int ret;

	ret = libxl_domain_info(CTX, &c_dominfo, Int_val(domid));
	if (ret != 0)
		failwith_xl(ERROR_FAIL, "domain_info");
	dominfo = Val_dominfo(&c_dominfo);

	CAMLreturn(dominfo);
}

value stub_xl_domain_sched_params_get(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal1(scinfo);
	libxl_domain_sched_params c_scinfo;
	int ret;

	ret = libxl_domain_sched_params_get(CTX, Int_val(domid), &c_scinfo);
	if (ret != 0)
		failwith_xl(ret, "domain_sched_params_get");

	scinfo = Val_domain_sched_params(&c_scinfo);

	libxl_domain_sched_params_dispose(&c_scinfo);

	CAMLreturn(scinfo);
}

value stub_xl_domain_sched_params_set(value ctx, value domid, value scinfo)
{
	CAMLparam3(ctx, domid, scinfo);
	libxl_domain_sched_params c_scinfo;
	int ret;

	domain_sched_params_val(CTX, &c_scinfo, scinfo);

	ret = libxl_domain_sched_params_set(CTX, Int_val(domid), &c_scinfo);

	libxl_domain_sched_params_dispose(&c_scinfo);

	if (ret != 0)
		failwith_xl(ret, "domain_sched_params_set");

	CAMLreturn(Val_unit);
}

value stub_xl_send_trigger(value ctx, value domid, value trigger, value vcpuid)
{
	CAMLparam4(ctx, domid, trigger, vcpuid);
	int ret;
	libxl_trigger c_trigger = LIBXL_TRIGGER_UNKNOWN;

	trigger_val(CTX, &c_trigger, trigger);

	ret = libxl_send_trigger(CTX, Int_val(domid),
				 c_trigger, Int_val(vcpuid));

	if (ret != 0)
		failwith_xl(ret, "send_trigger");

	CAMLreturn(Val_unit);
}

value stub_xl_send_sysrq(value ctx, value domid, value sysrq)
{
	CAMLparam3(ctx, domid, sysrq);
	int ret;

	ret = libxl_send_sysrq(CTX, Int_val(domid), Int_val(sysrq));

	if (ret != 0)
		failwith_xl(ret, "send_sysrq");

	CAMLreturn(Val_unit);
}

value stub_xl_send_debug_keys(value ctx, value keys)
{
	CAMLparam2(ctx, keys);
	int ret;
	char *c_keys;

	c_keys = dup_String_val(keys);

	ret = libxl_send_debug_keys(CTX, c_keys);
	free(c_keys);

	if (ret != 0)
		failwith_xl(ret, "send_debug_keys");

	CAMLreturn(Val_unit);
}

value stub_xl_xen_console_read(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal3(list, cons, ml_line);
	int i = 0, ret;
	char *console[32768], *line;
	libxl_xen_console_reader *cr;

	cr = libxl_xen_console_read_start(CTX, 0);
	if (cr)
		for (i = 0; libxl_xen_console_read_line(CTX, cr, &line) > 0; i++)
			console[i] = strdup(line);
	libxl_xen_console_read_finish(CTX, cr);

	list = Val_emptylist;
	for (; i > 0; i--) {
		ml_line = caml_copy_string(console[i - 1]);
		free(console[i - 1]);
		cons = caml_alloc(2, 0);
		Store_field(cons, 0, ml_line);  // head
		Store_field(cons, 1, list);     // tail
		list = cons;
	}

	CAMLreturn(list);
}


/* Event handling */

short Poll_val(value event)
{
	CAMLparam1(event);
	short res = -1;

	switch (Int_val(event)) {
		case 0: res = POLLIN; break;
		case 1: res = POLLPRI; break;
		case 2: res = POLLOUT; break;
		case 3: res = POLLERR; break;
		case 4: res = POLLHUP; break;
		case 5: res = POLLNVAL; break;
	}

	CAMLreturn(res);
}

short Poll_events_val(value event_list)
{
	CAMLparam1(event_list);
	short events = 0;

	while (event_list != Val_emptylist) {
		events |= Poll_val(Field(event_list, 0));
		event_list = Field(event_list, 1);
	}

	CAMLreturn(events);
}

value Val_poll(short event)
{
	CAMLparam0();
	CAMLlocal1(res);

	switch (event) {
		case POLLIN: res = Val_int(0); break;
		case POLLPRI: res = Val_int(1); break;
		case POLLOUT: res = Val_int(2); break;
		case POLLERR: res = Val_int(3); break;
		case POLLHUP: res = Val_int(4); break;
		case POLLNVAL: res = Val_int(5); break;
		default: failwith_xl(ERROR_FAIL, "cannot convert poll event value"); break;
	}

	CAMLreturn(res);
}

value add_event(value event_list, short event)
{
	CAMLparam1(event_list);
	CAMLlocal1(new_list);

	new_list = caml_alloc(2, 0);
	Store_field(new_list, 0, Val_poll(event));
	Store_field(new_list, 1, event_list);

	CAMLreturn(new_list);
}

value Val_poll_events(short events)
{
	CAMLparam0();
	CAMLlocal1(event_list);

	event_list = Val_emptylist;
	if (events & POLLIN)
		event_list = add_event(event_list, POLLIN);
	if (events & POLLPRI)
		event_list = add_event(event_list, POLLPRI);
	if (events & POLLOUT)
		event_list = add_event(event_list, POLLOUT);
	if (events & POLLERR)
		event_list = add_event(event_list, POLLERR);
	if (events & POLLHUP)
		event_list = add_event(event_list, POLLHUP);
	if (events & POLLNVAL)
		event_list = add_event(event_list, POLLNVAL);

	CAMLreturn(event_list);
}

int fd_register(void *user, int fd, void **for_app_registration_out,
                     short events, void *for_libxl)
{
	CAMLparam0();
	CAMLlocalN(args, 4);
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_fd_register");
	}

	args[0] = (value) user;
	args[1] = Val_int(fd);
	args[2] = Val_poll_events(events);
	args[3] = (value) for_libxl;

	caml_callbackN(*func, 4, args);
	CAMLreturn(0);
}

int fd_modify(void *user, int fd, void **for_app_registration_update,
                   short events)
{
	CAMLparam0();
	CAMLlocalN(args, 3);
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_fd_modify");
	}

	args[0] = (value) user;
	args[1] = Val_int(fd);
	args[2] = Val_poll_events(events);

	caml_callbackN(*func, 3, args);
	CAMLreturn(0);
}

void fd_deregister(void *user, int fd, void *for_app_registration)
{
	CAMLparam0();
	CAMLlocalN(args, 2);
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_fd_deregister");
	}

	args[0] = (value) user;
	args[1] = Val_int(fd);

	caml_callbackN(*func, 2, args);
	CAMLreturn0;
}

int timeout_register(void *user, void **for_app_registration_out,
                          struct timeval abs, void *for_libxl)
{
	CAMLparam0();
	CAMLlocalN(args, 4);
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_timeout_register");
	}

	args[0] = (value) user;
	args[1] = Val_int(abs.tv_sec);
	args[2] = Val_int(abs.tv_usec);
	args[3] = (value) for_libxl;

	caml_callbackN(*func, 4, args);
	CAMLreturn(0);
}

int timeout_modify(void *user, void **for_app_registration_update,
                         struct timeval abs)
{
	CAMLparam0();
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_timeout_modify");
	}

	caml_callback(*func, (value) user);
	CAMLreturn(0);
}

void timeout_deregister(void *user, void *for_app_registration)
{
	failwith_xl(ERROR_FAIL, "timeout_deregister not yet implemented");
	return;
}

value stub_libxl_osevent_register_hooks(value ctx, value user)
{
	CAMLparam2(ctx, user);
	CAMLlocal1(result);
	libxl_osevent_hooks *hooks;

	hooks = malloc(sizeof(*hooks));
	hooks->fd_register = fd_register;
	hooks->fd_modify = fd_modify;
	hooks->fd_deregister = fd_deregister;
	hooks->timeout_register = timeout_register;
	hooks->timeout_modify = timeout_modify;
	hooks->timeout_deregister = timeout_deregister;

	libxl_osevent_register_hooks(CTX, hooks, (void *) user);
	result = caml_alloc(1, Abstract_tag);
	*((libxl_osevent_hooks **) result) = hooks;

	CAMLreturn(result);
}

value stub_libxl_osevent_occurred_fd(value ctx, value for_libxl, value fd,
	value events, value revents)
{
	CAMLparam5(ctx, for_libxl, fd, events, revents);
	libxl_osevent_occurred_fd(CTX, (void *) for_libxl, Int_val(fd),
		Poll_events_val(events), Poll_events_val(revents));
	CAMLreturn(Val_unit);
}

value stub_libxl_osevent_occurred_timeout(value ctx, value for_libxl)
{
	CAMLparam2(ctx, for_libxl);
	libxl_osevent_occurred_timeout(CTX, (void *) for_libxl);
	CAMLreturn(Val_unit);
}

struct user_with_ctx {
	libxl_ctx *ctx;
	void *user;
};

void event_occurs(void *user, const libxl_event *event)
{
	CAMLparam0();
	CAMLlocalN(args, 2);
	struct user_with_ctx *c_user = (struct user_with_ctx *) user;
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_event_occurs_callback");
	}

	args[0] = (value) c_user->user;
	args[1] = Val_event((libxl_event *) event);
	libxl_event_free(c_user->ctx, (libxl_event *) event);

	caml_callbackN(*func, 2, args);
	CAMLreturn0;
}

void disaster(void *user, libxl_event_type type,
                     const char *msg, int errnoval)
{
	CAMLparam0();
	CAMLlocalN(args, 2);
	struct user_with_ctx *c_user = (struct user_with_ctx *) user;
	static value *func = NULL;

	if (func == NULL) {
		/* First time around, lookup by name */
		func = caml_named_value("libxl_event_disaster_callback");
	}

	args[0] = (value) c_user->user;
	args[1] = Val_event_type(type);
	args[2] = caml_copy_string(msg);
	args[3] = Val_int(errnoval);

	caml_callbackN(*func, 4, args);
	CAMLreturn0;
}

value stub_libxl_event_register_callbacks(value ctx, value user)
{
	CAMLparam2(ctx, user);
	CAMLlocal1(result);
	struct user_with_ctx *c_user = NULL;
	libxl_event_hooks *hooks;

	c_user = malloc(sizeof(*c_user));
	c_user->user = (void *) user;
	c_user->ctx = CTX;

	hooks = malloc(sizeof(*hooks));
	hooks->event_occurs_mask = LIBXL_EVENTMASK_ALL;
	hooks->event_occurs = event_occurs;
	hooks->disaster = disaster;

	libxl_event_register_callbacks(CTX, (const libxl_event_hooks *) hooks, (void *) c_user);
	result = caml_alloc(1, Abstract_tag);
	*((libxl_event_hooks **) result) = hooks;

	CAMLreturn(result);
}

value stub_libxl_evenable_domain_death(value ctx, value domid, value user)
{
	CAMLparam3(ctx, domid, user);
	libxl_evgen_domain_death *evgen_out;

	libxl_evenable_domain_death(CTX, Int_val(domid), Int_val(user), &evgen_out);

	CAMLreturn(Val_unit);
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
