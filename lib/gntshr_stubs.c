/*
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
 */

#include <stdlib.h>
#include <errno.h>

#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/signals.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/bigarray.h>

#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <xenctrl.h>

#if __XEN_INTERFACE_VERSION__ >= 0x00040200
#define HAVE_GNTSHR 1
#endif

static void gntshr_missing()
{
	value *v = caml_named_value("gntshr.missing");
	/* if v is NULL then it's either an error in gntshr.ml or a
	   linking error */ 
	assert (v != NULL);
	caml_raise_constant(*v);
}

#define _G(__g) ((xc_gntshr *)(__g))

#define XC_GNTTAB_BIGARRAY (CAML_BA_UINT8 | CAML_BA_C_LAYOUT | CAML_BA_EXTERNAL)

#ifdef HAVE_GNTSHR
#define ERROR_STRLEN 1024
static void failwith_xc(xc_interface *xch)
{
        static char error_str[ERROR_STRLEN];
        if (xch) {
                const xc_error *error = xc_get_last_error(xch);
                if (error->code == XC_ERROR_NONE)
                        snprintf(error_str, ERROR_STRLEN, "%d: %s", errno, strerror(errno));
                else
                        snprintf(error_str, ERROR_STRLEN, "%d: %s: %s",
                                 error->code,
                                 xc_error_code_to_desc(error->code),
                                 error->message);
        } else {
                snprintf(error_str, ERROR_STRLEN, "Unable to open XC interface");
        }
        caml_failwith(error_str);
}
#endif

CAMLprim value stub_xc_gntshr_open(value unit)
{
	CAMLparam1(unit);
	CAMLlocal1(result);
#ifdef HAVE_GNTSHR
	xc_gntshr *xgh;

	xgh = xc_gntshr_open(NULL, 0);
	if (NULL == xgh)
		failwith_xc(NULL);
	result = (value)xgh;
#else
	gntshr_missing();
#endif
	CAMLreturn(result);
}

CAMLprim value stub_xc_gntshr_close(value xgh)
{
	CAMLparam1(xgh);
#ifdef HAVE_GNTSHR
	xc_gntshr_close(_G(xgh));
#else
	gntshr_missing();
#endif
	CAMLreturn(Val_unit);
}

CAMLprim value stub_xc_gntshr_share_pages(value xgh, value domid, value count, value writeable) {
	CAMLparam4(xgh, domid, count, writeable);
	CAMLlocal4(result, ml_refs, ml_refs_cons, ml_map);
#ifdef HAVE_GNTSHR
	void *map;
	uint32_t *refs;
	int i;
  int c_count = Int_val(count);
	result = caml_alloc(2, 0);
	refs = malloc(c_count * sizeof(uint32_t));

	map = xc_gntshr_share_pages(_G(xgh), Int_val(domid), c_count, refs, Bool_val(writeable));

	if(NULL == map) {
		free(refs);
		failwith_xc(_G(xgh));
	}

	// Construct the list of grant references.
	ml_refs = Val_emptylist;
	for(i = c_count - 1; i >= 0; i--) {
		ml_refs_cons = caml_alloc(2, 0);

		Store_field(ml_refs_cons, 0, Val_int(refs[i]));
		Store_field(ml_refs_cons, 1, ml_refs);

		ml_refs = ml_refs_cons;
	}

	ml_map = alloc_bigarray_dims(XC_GNTTAB_BIGARRAY, 1,
                              map, (c_count << XC_PAGE_SHIFT));

	Store_field(result, 0, ml_refs);
	Store_field(result, 1, ml_map);

	free(refs);
#else
	gntshr_missing();
#endif
	CAMLreturn(result);
}

CAMLprim value stub_xc_gntshr_munmap(value xgh, value share) {
	CAMLparam2(xgh, share);
	CAMLlocal1(ml_map);
#ifdef HAVE_GNTSHR
	ml_map = Field(share, 1);

	int size = Bigarray_val(ml_map)->dim[0];
	int pages = size >> XC_PAGE_SHIFT;
	int result = xc_gntshr_munmap(_G(xgh), Data_bigarray_val(ml_map), pages);
	if(result != 0)
		failwith_xc(_G(xgh));
#else
	gntshr_missing();
#endif
	CAMLreturn(Val_unit);
}
