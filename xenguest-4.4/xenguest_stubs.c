/*
 * Copyright (C) 2006-2009 Citrix Systems Inc.
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include <errno.h>
#include <sys/mman.h>

#include <xenctrl.h>
#include <xenguest.h>
#include <xenstore.h>
/*
#include <xc_dom.h>
*/
#include <xen/hvm/hvm_info_table.h>
#include <xen/hvm/params.h>
#include <xen/hvm/e820.h>

#include "xg_internal.h"
#include "config.h"
#include "../config.h"

/* The following boolean flags are all set by their value
   in the platform area of xenstore. The only value that
   is considered true is the string 'true' */
struct flags {
    int vcpus;
    int vcpus_current;
    char** vcpu_affinity; /* 0 means unset */
    uint16_t vcpu_weight;   /* 0 means unset (0 is an illegal weight) */
    uint16_t vcpu_cap;      /* 0 is default (no cap) */
    int nx;
    int viridian;
    int pae;
    int acpi;
    int apic;
    int acpi_s3;
    int acpi_s4;
    int tsc_mode;
    int hpet;
    int nested_hvm;
};

static char *
xenstore_getsv(xs_handle *xsh, int domid, const char *fmt, va_list ap)
{
    char *path = NULL, *s = NULL;
    int n, m;
    char key[1024] = { 0 };

    path = xs_get_domain_path(xsh, domid);
    if (path == NULL)
        goto out;

    n = snprintf(key, sizeof(key), "%s/", path);
    if (n < 0)
        goto out;
    m = vsnprintf(key + n, sizeof(key) - n, fmt, ap);
    if (m < 0)
        goto out;

    s = xs_read(xsh, XBT_NULL, key, NULL);
out:
    free(path);
    return s;
}

static char *
xenstore_gets(xs_handle *xsh, int domid, const char *fmt, ...)
{
    char *s;
    va_list ap;

    va_start(ap, fmt);
    s = xenstore_getsv(xsh, domid, fmt, ap);
    va_end(ap);
    return s;
}

static uint64_t
xenstore_get(xs_handle *xsh, int domid, const char *fmt, ...)
{
    char *s;
    uint64_t value = 0;
    va_list ap;

    va_start(ap, fmt);
    s = xenstore_getsv(xsh, domid, fmt, ap);
    if (s) {
        if (!strcasecmp(s, "true"))
            value = 1;
        else
        {
            errno = 0;
            value = strtoull(s, NULL, 0);
            if ( errno )
                value = 0;
        }
        free(s);
    }
    va_end(ap);
    return value;
}

static int
xenstore_putsv(xs_handle *xsh, int domid, const char *_key, const char *fmt, ...)
{
    int n, m, rc = -1;
    char *path, key[512], val[512];
    va_list ap;

    path = xs_get_domain_path(xsh, domid);
    if (path == NULL)
        goto out;

    n = snprintf(key, sizeof(key), "%s/%s", path, _key);
    if (n < 0)
        goto out;

    va_start(ap, fmt);
    m = vsnprintf(val, sizeof(val), fmt, ap);
    va_end(ap);
    if (m < 0)
        goto out;

    rc = xs_write(xsh, XBT_NULL, key, val, strlen(val));
out:
    free(path);
    return rc;
}

static int
xenstore_puts(xs_handle *xsh, int domid, const char *key, const char *val)
{
    return xenstore_putsv(xsh, domid, key, "%s", val);
}

static int hvmloader_flag(xs_handle *xsh, int domid, const char *key)
{
    /* Params going to hvmloader need to convert "true" -> '1' as Xapi gets
     * this wrong when migrating from older hosts. */

    char *val = xenstore_gets(xsh, domid, key);
    int ret = -1;

    if ( val )
    {
        if ( !strcmp(val, "1") )
            return 1;
        else if ( !strcmp(val, "0") )
            return 0;
        if ( !strcasecmp(val, "true") )
            ret = 1;
        else
        {
            errno = 0;
            ret = strtol(val, NULL, 0);
            if ( errno )
                ret = 0;
        }

        err("HVMLoader error: Fixing up key '%s' from '%s' to '%d'", key, val, ret);
        xenstore_putsv(xsh, domid, key, "%d", !!ret);
    }

    return ret;
}

static void
get_flags(xs_handle *xsh, struct flags *f, int domid)
{
    char * tmp;
    int n;
    f->vcpus = xenstore_get(xsh, domid, "platform/vcpu/number");
    f->vcpu_affinity = malloc(sizeof(char*) * f->vcpus);

    for (n = 0; n < f->vcpus; n++) {
        f->vcpu_affinity[n] = xenstore_gets(xsh, domid, "platform/vcpu/%d/affinity", n);
    }
    f->vcpus_current = xenstore_get(xsh, domid, "platform/vcpu/current");
    f->vcpu_weight = xenstore_get(xsh, domid, "platform/vcpu/weight");
    f->vcpu_cap = xenstore_get(xsh, domid, "platform/vcpu/cap");
    f->nx       = xenstore_get(xsh, domid, "platform/nx");
    f->viridian = xenstore_get(xsh, domid, "platform/viridian");
    f->apic     = xenstore_get(xsh, domid, "platform/apic");
    f->pae      = xenstore_get(xsh, domid, "platform/pae");
    f->tsc_mode = xenstore_get(xsh, domid, "platform/tsc_mode");
    f->nested_hvm = xenstore_get(xsh, domid, "platform/exp-nested-hvm");

    /* Params going to hvmloader - need to convert "true" -> '1' as Xapi gets
     * this wrong when migrating from older hosts. */
    f->acpi    = hvmloader_flag(xsh, domid, "platform/acpi");
    f->acpi_s4 = hvmloader_flag(xsh, domid, "platform/acpi_s4");
    f->acpi_s3 = hvmloader_flag(xsh, domid, "platform/acpi_s3");

    /*
     * HACK - Migrated VMs wont have this xs key set, so the naive action
     * would result in the HPET mysteriously disappearing.  If the key is not
     * present then enable the hpet to match its default.
     */
    tmp = xenstore_gets(xsh, domid, "platform/hpet");
    if ( tmp && strlen(tmp) )
        f->hpet = xenstore_get(xsh, domid, "platform/hpet");
    else
        f->hpet = 1;
    free(tmp);

    info("Determined the following parameters from xenstore:\n");
    info("vcpu/number:%d vcpu/weight:%d vcpu/cap:%d nx: %d viridian: %d "
         "apic: %d acpi: %d pae: %d acpi_s4: %d acpi_s3: %d tsc_mode: %d "
         "hpet: %d\n",
         f->vcpus, f->vcpu_weight, f->vcpu_cap, f->nx, f->viridian, f->apic,
         f->acpi, f->pae, f->acpi_s4, f->acpi_s3, f->tsc_mode, f->hpet);
    for (n = 0; n < f->vcpus; n++){
        info("vcpu/%d/affinity:%s\n", n, (f->vcpu_affinity[n])?f->vcpu_affinity[n]:"unset");
    }
}

static void free_flags(struct flags *f)
{
    int n;
    for ( n = 0; n < f->vcpus; ++n )
        free(f->vcpu_affinity[n]);
    free(f->vcpu_affinity);
}

static void failwith_oss_xc(xc_interface *xch, char *fct)
{
    char buf[80];
    const xc_error *error;

    error = xc_get_last_error(xch);
    if (error->code == XC_ERROR_NONE)
        snprintf(buf, 80, "%s: [%d] %s", fct, errno, strerror(errno));
    else
        snprintf(buf, 80, "%s: [%d] %s", fct, error->code, error->message);
    xc_clear_last_error(xch);
    err("xenguest: %s\n", buf);
    exit(1);
}

extern struct xc_dom_image *xc_dom_allocate(xc_interface *xch, const char *cmdline, const char *features);

static void configure_vcpus(xc_interface *xch, int domid, struct flags f){
    struct xen_domctl_sched_credit sdom;
    int i, j, r, size, pcpus_supplied, min;
    xc_cpumap_t cpumap;

    size = xc_get_cpumap_size(xch) * 8; /* array is of uint8_t */

    for (i=0; i<f.vcpus; i++){
        if (f.vcpu_affinity[i]){ /* NULL means unset */
            pcpus_supplied = strlen(f.vcpu_affinity[i]);
            min = (pcpus_supplied < size)?pcpus_supplied:size;
            cpumap = xc_cpumap_alloc(xch);
            if (cpumap == NULL)
                failwith_oss_xc(xch, "xc_cpumap_alloc");

            for (j=0; j<min; j++) {
                if (f.vcpu_affinity[i][j] == '1')
                    cpumap[j/8] |= 1 << (j&7);
            }
            r = xc_vcpu_setaffinity(xch, domid, i, cpumap);
            free(cpumap);
            if (r) {
                failwith_oss_xc(xch, "xc_vcpu_setaffinity");
            }
        }
    }

    r = xc_sched_credit_domain_get(xch, domid, &sdom);
    /* This should only happen when a different scheduler is set */
    if (r) {
        info("Failed to get credit scheduler parameters: scheduler not enabled?\n");
        return;
    }
    if (f.vcpu_weight != 0L) sdom.weight = f.vcpu_weight;
    if (f.vcpu_cap != 0L) sdom.cap = f.vcpu_cap;
    /* This shouldn't fail, if "get" above succeeds. This error is fatal
       to highlight the need to investigate further. */
    r = xc_sched_credit_domain_set(xch, domid, &sdom);
    if (r)
        failwith_oss_xc(xch, "xc_sched_credit_domain_set");
}

static void configure_tsc(xc_interface *xch, int domid, struct flags f){
    xc_domain_set_tsc_info(xch, domid, f.tsc_mode, 0, 0, 0);
}


int stub_xc_linux_build(xc_interface *xch, xs_handle *xsh, int domid,
                        int c_mem_max_mib, int mem_start_mib,
                        const char *image_name, const char *ramdisk_name,
                        const char *cmdline, const char *features,
                        int flags, int store_evtchn, int store_domid,
                        int console_evtchn, int console_domid,
                        unsigned long *store_mfn, unsigned long *console_mfn,
                        char *protocol)
{
    int r;
    struct xc_dom_image *dom;

    struct flags f;
    get_flags(xsh, &f, domid);

    xc_dom_loginit(xch);
    dom = xc_dom_allocate(xch, cmdline, features);
    if (!dom)
        failwith_oss_xc(xch, "xc_dom_allocate");

    configure_vcpus(xch, domid, f);
    configure_tsc(xch, domid, f);

    r = xc_dom_linux_build(xch, dom, domid, mem_start_mib,
                           image_name, ramdisk_name, flags,
                           store_evtchn, store_mfn,
                           console_evtchn, console_mfn);

    if (r == 0)
        r = xc_dom_gnttab_seed(xch, domid,
                               *console_mfn, *store_mfn,
                               console_domid, store_domid);


    strncpy(protocol, xc_domain_get_native_protocol(xch, domid), 64);

    free_flags(&f);
    xc_dom_release(dom);

    if (r != 0)
        failwith_oss_xc(xch, "xc_dom_linux_build");

    return 0;
}


static int hvm_build_set_params(xc_interface *xch, int domid,
                                int store_evtchn, unsigned long *store_mfn,
                                int console_evtchn, unsigned long *console_mfn,
                                struct flags f)
{
    struct hvm_info_table *va_hvm;
    uint8_t *va_map, sum;
    uint32_t i;

    va_map = xc_map_foreign_range(xch, domid,
                                  XC_PAGE_SIZE, PROT_READ | PROT_WRITE,
                                  HVM_INFO_PFN);
    if (va_map == NULL)
        return -1;

    va_hvm = (struct hvm_info_table *)(va_map + HVM_INFO_OFFSET);
    va_hvm->apic_mode = f.apic;
    va_hvm->nr_vcpus = f.vcpus;
    memset(va_hvm->vcpu_online, 0, sizeof(va_hvm->vcpu_online));
    for (i = 0; i < f.vcpus_current; i++)
        va_hvm->vcpu_online[i/8] |= 1 << (i % 8);
    va_hvm->checksum = 0;
    for (i = 0, sum = 0; i < va_hvm->length; i++)
        sum += ((uint8_t *) va_hvm)[i];
    va_hvm->checksum = -sum;
    munmap(va_map, XC_PAGE_SIZE);

    xc_get_hvm_param(xch, domid, HVM_PARAM_STORE_PFN, store_mfn);
    xc_set_hvm_param(xch, domid, HVM_PARAM_PAE_ENABLED, f.pae);
#ifdef HAVE_HVM_PARAM_VIRIDIAN
    xc_set_hvm_param(xch, domid, HVM_PARAM_VIRIDIAN, f.viridian);
#endif
    xc_set_hvm_param(xch, domid, HVM_PARAM_STORE_EVTCHN, store_evtchn);
    xc_set_hvm_param(xch, domid, HVM_PARAM_HPET_ENABLED, f.hpet);
    xc_set_hvm_param(xch, domid, HVM_PARAM_NESTEDHVM, f.nested_hvm);
#ifdef HAVE_HVM_PARAM_NX_ENABLED
    xc_set_hvm_param(xch, domid, HVM_PARAM_NX_ENABLED, f.nx);
#endif
    xc_get_hvm_param(xch, domid, HVM_PARAM_CONSOLE_PFN, console_mfn);
    xc_set_hvm_param(xch, domid, HVM_PARAM_CONSOLE_EVTCHN, console_evtchn);
    return 0;
}

int stub_xc_hvm_build(xc_interface *xch, xs_handle *xsh, int domid,
                      int mem_max_mib, int mem_start_mib, const char *image_name,
                      int store_evtchn, int store_domid,
                      int console_evtchn, int console_domid,
                      unsigned long *store_mfn, unsigned long *console_mfn)
{
    int r;
    struct flags f;
    get_flags(xsh, &f, domid);

    configure_vcpus(xch, domid, f);
    configure_tsc(xch, domid, f);

    r = xc_hvm_build_target_mem(xch, domid,
                                mem_max_mib,
                                mem_start_mib,
                                image_name);
    if (r)
        failwith_oss_xc(xch, "hvm_build");

    r = hvm_build_set_params(xch, domid, store_evtchn, store_mfn,
                             console_evtchn, console_mfn, f);
    if (r)
        failwith_oss_xc(xch, "hvm_build_params");

    xc_dom_gnttab_hvm_seed(xch, domid, *console_mfn, *store_mfn,
                           console_domid, store_domid);

    free_flags(&f);

    return 0;
}

struct save_callbacks_data
{
    xs_handle *xsh;
    int domid;
};

static int dispatch_suspend(void *_data)
{
    struct save_callbacks_data * data = _data;
    return suspend_callback(data->domid);
}

static int switch_qemu_logdirty(int domid, unsigned enable, void *_data)
{
    struct save_callbacks_data * data = _data;
    char *path = NULL;
    bool rc;

    asprintf(&path, "/local/domain/0/device-model/%u/logdirty/cmd", data->domid);

    if (enable)
        rc = xs_write(data->xsh, XBT_NULL, path, "enable", strlen("enable"));
    else
        rc = xs_write(data->xsh, XBT_NULL, path, "disable", strlen("disable"));

    free(path);
    return rc ? 0 : 1;

}

#define GENERATION_ID_ADDRESS "hvmloader/generation-id-address"

int stub_xc_domain_save(xc_interface *xch, xs_handle *xsh, int fd, int domid,
                        int max_iters, int max_factors,
                        int flags, int hvm)
{
    int r;
    struct save_callbacks_data cb_data =
        { xsh, domid };
    struct save_callbacks callbacks =
        {
            .suspend = dispatch_suspend,
            .switch_qemu_logdirty = switch_qemu_logdirty,
            .data = &cb_data
        };
    uint64_t generation_id_addr = xenstore_get(xsh, domid, GENERATION_ID_ADDRESS);

    r = xc_domain_save(xch, fd, domid,
                       max_iters, max_factors,
                       flags, &callbacks, hvm, generation_id_addr);
    if (r)
        failwith_oss_xc(xch, "xc_domain_save");

    return 0;
}

/* this is the slow version of resume for uncooperative domain,
 * the fast version is available in close source xc */
int stub_xc_domain_resume_slow(xc_interface *xch, xs_handle *xsh, int domid)
{
    int r;

    /* hard code fast to 0, we only want to expose the slow version here */
    r = xc_domain_resume(xch, domid, 0);
    if (r)
        failwith_oss_xc(xch, "xc_domain_resume");
    return 0;
}

struct restore_callbacks_data
{
    xs_handle *xsh;
};

int toolstack_restore_cb(uint32_t _domid, const uint8_t *buf,
    uint32_t size, void* _data)
{
    info("TODO: restore generation id for domain %d", _domid);
    return 0;
}

int stub_xc_domain_restore(xc_interface *xch, xs_handle *xsh, int fd, int domid,
                           int store_evtchn, int console_evtchn,
                           int hvm,
                           unsigned long *store_mfn, unsigned long *console_mfn)
{
    int r;
    struct flags f;

    struct restore_callbacks_data rcbd =
    {
        .xsh = xsh,
    };

    struct restore_callbacks rcb =
    {
        .toolstack_restore = toolstack_restore_cb,
        .data = &rcbd,
    };

    get_flags(xsh, &f, domid);

    if ( hvm )
    {
#ifdef HAVE_HVM_PARAM_VIRIDIAN
        xc_set_hvm_param(xch, domid, HVM_PARAM_VIRIDIAN, f.viridian);
#endif
        xc_set_hvm_param(xch, domid, HVM_PARAM_HPET_ENABLED, f.hpet);
    }

    configure_vcpus(xch, domid, f);

    r = xc_domain_restore(xch, fd, domid,
                          store_evtchn, store_mfn, 0,
                          console_evtchn, console_mfn, 0,
                          hvm, f.pae, /* int superpages */ 0,
                          /* int no_incr_generation_id */ 0,
                          /* int checkpointed_stream */0,
                          /* unsigned long *vm_generationid_addr */NULL,
                          &rcb);

    free_flags(&f);
    if (r)
        failwith_oss_xc(xch, "xc_domain_restore");
    return 0;
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
