/*
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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <string.h>

#include <xenctrl.h>
#include <xen/xen.h>
#include <xen/sys/evtchn.h>

#define CAML_NAME_SPACE
#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/custom.h>
#include <caml/callback.h>
#include <caml/fail.h>

#define _H(__h) ((xc_evtchn *)(__h))

xc_interface *global_xce = NULL;

CAMLprim value stub_eventchn_init(value unit)
{
  CAMLparam1(unit);
  if (global_xce == NULL) {
    global_xce = xc_evtchn_open(NULL, XC_OPENFLAG_NON_REENTRANT);
  }

  if (global_xce == NULL)
    caml_failwith(strerror(errno));

  CAMLreturn((value)global_xce);
}

CAMLprim value stub_eventchn_close(value xce)
{
/*  return Val_int(xc_evtchn_close(_H(xce))); */
/* We don't ever actually want to do this */
  return(Val_int(0));
}

CAMLprim value stub_eventchn_fd(value xce)
{
  CAMLparam1(xce);
  int fd;

  fd = xc_evtchn_fd(_H(xce));
  if (fd == -1)
    {
      perror("xc_evtchn_fd");
      caml_failwith(strerror(errno));
    }
  CAMLreturn(Val_int(fd));
}

CAMLprim value stub_eventchn_notify(value xce, value port)
{
  CAMLparam2(xce, port);
  if(xc_evtchn_notify(_H(xce), Int_val(port)) == -1)
    {
      perror("xc_evtchn_notify");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_unit);
}

CAMLprim value stub_eventchn_bind_interdomain(value xce, value domid,
                                              value remote_port)
{
  CAMLparam3(xce, domid, remote_port);
  evtchn_port_or_error_t rc;

  rc = xc_evtchn_bind_interdomain(_H(xce), Int_val(domid), Int_val(remote_port));
  if (rc == -1)
    {
      perror("xc_evtchn_bind_interdomain");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_int(rc));
}

CAMLprim value stub_eventchn_bind_unbound_port(value xce, value remote_domid)
{
  CAMLparam2(xce, remote_domid);
  evtchn_port_or_error_t rc;

  rc = xc_evtchn_bind_unbound_port(_H(xce), Int_val(remote_domid));
  if (rc == -1)
    {
      perror("xc_evtchn_bind_unbound_port");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_int(rc));

}

CAMLprim value stub_eventchn_bind_dom_exc_virq(value xce)
{
  CAMLparam1(xce);
  evtchn_port_or_error_t rc;

  rc = xc_evtchn_bind_virq(_H(xce), VIRQ_DOM_EXC);
  if (rc == -1)
    {
      perror("xc_evtchn_bind_virq");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_int(rc));
}

CAMLprim value stub_eventchn_unbind(value xce, value port)
{
  CAMLparam2(xce, port);
  if(xc_evtchn_unbind(_H(xce), Int_val(port)) == -1)
    {
      perror("xc_evtchn_unbind");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_unit);
}

CAMLprim value stub_eventchn_pending(value xce)
{
  CAMLparam1(xce);
  evtchn_port_or_error_t port;

  port = xc_evtchn_pending(_H(xce));
  if (port == -1)
    {
      perror("xc_evtchn_pending");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_int(port));
}

CAMLprim value stub_eventchn_unmask(value xce, value port)
{
  CAMLparam2(xce, port);
  if (xc_evtchn_unmask(_H(xce), Int_val(port)) == -1)
    {
      perror("xc_evtchn_unmask");
      caml_failwith(strerror(errno));
    }

  CAMLreturn(Val_unit);
}
