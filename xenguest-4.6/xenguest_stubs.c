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
#include <unistd.h>

#include <xenctrl.h>
#include <xenguest.h>
#include <xenstore.h>
/*
#include <xc_dom.h>
*/
/* missing declarations from xc_dom.h */
int xc_dom_loginit(xc_interface *xch);
int xc_dom_gnttab_init(struct xc_dom_image *dom);
int xc_dom_gnttab_hvm_seed(xc_interface *xch, domid_t domid,
                           xen_pfn_t console_gmfn,
                           xen_pfn_t xenstore_gmfn,
                           domid_t console_domid,
                           domid_t xenstore_domid);
int xc_dom_gnttab_seed(xc_interface *xch, domid_t domid,
                       xen_pfn_t console_gmfn,
                       xen_pfn_t xenstore_gmfn,
                       domid_t console_domid,
                       domid_t xenstore_domid);
int xc_dom_kernel_max_size(struct xc_dom_image *dom, size_t sz);
int xc_dom_ramdisk_max_size(struct xc_dom_image *dom, size_t sz);
void xc_dom_release(struct xc_dom_image *dom);

#include <xen/hvm/hvm_info_table.h>
#include <xen/hvm/params.h>
#include <xen/hvm/e820.h>

#ifdef XEN_SYSCTL_cpu_featureset_raw
#include <xen/arch-x86/featureset.h>
#endif

#include "xg_internal.h"
/*
#include "xc_bitops.h"
*/
char *xs_domain_path = NULL;
char *pci_passthrough_sbdf_list = NULL;

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
    int viridian_time_ref_count;
    int viridian_reference_tsc;
    int pae;
    int acpi;
    int apic;
    int acpi_s3;
    int acpi_s4;
    int tsc_mode;
    int hpet;
    int nested_hvm;
    unsigned cores_per_socket;
};

char *xenstore_getsv(const char *fmt, va_list ap)
{
    char *s = NULL;
    int n, m;
    char key[1024] = { 0 };

    n = snprintf(key, sizeof(key), "%s/", xs_domain_path);
    if (n < 0)
        goto out;
    m = vsnprintf(key + n, sizeof(key) - n, fmt, ap);
    if (m < 0)
        goto out;

    s = xs_read(xsh, XBT_NULL, key, NULL);
out:
    return s;
}

char *xenstore_gets(const char *fmt, ...)
{
    char *s;
    va_list ap;

    va_start(ap, fmt);
    s = xenstore_getsv(fmt, ap);
    va_end(ap);
    return s;
}

uint64_t xenstore_get(const char *fmt, ...)
{
    char *s;
    uint64_t value = 0;
    va_list ap;

    va_start(ap, fmt);
    s = xenstore_getsv(fmt, ap);
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

int xenstore_putsv(const char *_key, const char *fmt, ...)
{
    int n, m, rc = -1;
    char key[512], val[512];
    va_list ap;

    n = snprintf(key, sizeof(key), "%s/%s", xs_domain_path, _key);
    if (n < 0)
        goto out;

    va_start(ap, fmt);
    m = vsnprintf(val, sizeof(val), fmt, ap);
    va_end(ap);
    if (m < 0)
        goto out;

    rc = xs_write(xsh, XBT_NULL, key, val, strlen(val));
out:
    return rc;
}

int xenstore_puts(const char *key, const char *val)
{
    return xenstore_putsv(key, "%s", val);
}

static uint32_t *featureset, nr_features;

/*
 * Choose the featureset to use for a VM.
 *
 * The toolstack is expected to provide a featureset in the
 * platform/featureset xenstore key, fomatted as a bitmap of '-' delimited
 * 32bit hex-encoded words.  e.g.
 *
 *   aaaaaaaa-bbbbbbbb-cccccccc
 *
 * If no featureset is found, default to the host maximum.  It is important in
 * a heterogenous case to permit featuresets longer than this hosts maximum,
 * if they have been zero-extended to make a common longest length.
 */
static int get_vm_featureset(bool hvm)
{
#ifdef XEN_SYSCTL_cpu_featureset_raw
    char *platform = xenstore_gets("platform/featureset");
    char *s = platform, *e;
    unsigned int i = 0;
    int rc = 0;

    if ( !platform )
    {
        xg_info("No featureset provided - using host maximum\n");

        return xc_get_cpu_featureset(xch,
                                     hvm ? XEN_SYSCTL_cpu_featureset_hvm
                                         : XEN_SYSCTL_cpu_featureset_pv,
                                     &nr_features, featureset);
    }
    else
        xg_info("Parsing '%s' as featureset\n", platform);

    while ( *s != '\0' )
    {
        unsigned long val;

        errno = 0;
        val = strtoul(s, &e, 16);
        if ( (errno != 0) ||            /* Error converting. */
             (val > ~(uint32_t)0) ||    /* Value out of range. */
             (e == s) ||                /* No digits found. */
                                        /* Bad following characters. */
             !(*e == '\0' || *e == '-' || *e == ':')
            )
        {
            xg_err("Bad '%s' in featureset\n", s);
            rc = -1;
            break;
        }

        if ( i < nr_features )
            featureset[i++] = val;
        else if ( val != 0 )
        {
            xg_err("Requested featureset '%s' truncated on this host\n", platform);
            rc = -1;
            break;
        }

        s = e;
        if ( *s == '-' || *s == ':' )
            s++;
    }

    free(platform);
    return rc;
#else
    return 0;
#endif
}

int construct_cpuid_policy(const struct flags *f, bool hvm)
{
#ifdef XEN_SYSCTL_cpu_featureset_raw
    int rc = -1;

    if ( xc_get_cpu_featureset(xch,
                               XEN_SYSCTL_cpu_featureset_host,
                               &nr_features, NULL) ||
         nr_features == 0 )
    {
        xg_err("Failed to obtain featureset size %d %s\n",
               errno, strerror(errno));
        goto out;
    }

    featureset = calloc(nr_features, sizeof(*featureset));
    if ( !featureset )
    {
        xg_err("Failed to allocate memory for featureset\n");
        goto out;
    }

    if ( get_vm_featureset(hvm) )
        goto out;

    if ( !f->nx )
        clear_bit(X86_FEATURE_NX, featureset);

    rc = xc_cpuid_apply_policy(xch, domid, featureset, nr_features);

 out:
    free(featureset);
    featureset = NULL;
    return rc;
#else
    return 0;
#endif
}

static int hvmloader_flag(const char *key)
{
    /* Params going to hvmloader need to convert "true" -> '1' as Xapi gets
     * this wrong when migrating from older hosts. */

    char *val = xenstore_gets(key);
    int ret = -1;

    if ( val )
    {
        if ( !strcmp(val, "1") )
        {
            ret = 1;
            goto out;
        }
        else if ( !strcmp(val, "0") )
        {
            ret = 0;
            goto out;
        }
        if ( !strcasecmp(val, "true") )
            ret = 1;
        else
        {
            errno = 0;
            ret = strtol(val, NULL, 0);
            if ( errno )
                ret = 0;
        }

        xg_info("HVMLoader error: Fixing up key '%s' from '%s' to '%d'\n", key, val, ret);
        xenstore_putsv(key, "%d", !!ret);
    }
    else
        xenstore_puts(key, "0");

 out:
    free(val);
    return ret;
}

static void get_flags(struct flags *f)
{
    char * tmp;
    int n;
    f->vcpus = xenstore_get("platform/vcpu/number");
    f->vcpu_affinity = malloc(sizeof(char*) * f->vcpus);

    for (n = 0; n < f->vcpus; n++) {
        f->vcpu_affinity[n] = xenstore_gets("platform/vcpu/%d/affinity", n);
    }
    f->vcpus_current = xenstore_get("platform/vcpu/current");
    f->vcpu_weight = xenstore_get("platform/vcpu/weight");
    f->vcpu_cap = xenstore_get("platform/vcpu/cap");
    f->nx       = xenstore_get("platform/nx");
    f->viridian = xenstore_get("platform/viridian");
    f->viridian_time_ref_count = xenstore_get("platform/viridian_time_ref_count");
    f->viridian_reference_tsc = xenstore_get("platform/viridian_reference_tsc");
    f->apic     = xenstore_get("platform/apic");
    f->pae      = xenstore_get("platform/pae");
    f->tsc_mode = xenstore_get("platform/tsc_mode");
    f->nested_hvm = xenstore_get("platform/exp-nested-hvm");
    f->cores_per_socket = xenstore_get("platform/cores-per-socket");

    /* Params going to hvmloader - need to convert "true" -> '1' as Xapi gets
     * this wrong when migrating from older hosts. */
    f->acpi    = hvmloader_flag("platform/acpi");
    f->acpi_s4 = hvmloader_flag("platform/acpi_s4");
    f->acpi_s3 = hvmloader_flag("platform/acpi_s3");

    /*
     * HACK - Migrated VMs wont have this xs key set, so the naive action
     * would result in the HPET mysteriously disappearing.  If the key is not
     * present then enable the hpet to match its default.
     */
    tmp = xenstore_gets("platform/hpet");
    if ( tmp && strlen(tmp) )
        f->hpet = xenstore_get("platform/hpet");
    else
        f->hpet = 1;
    free(tmp);

    xg_info("Determined the following parameters from xenstore:\n");
    xg_info("vcpu/number:%d vcpu/weight:%d vcpu/cap:%d\n",
            f->vcpus, f->vcpu_weight, f->vcpu_cap);
    xg_info("nx: %d, pae %d, cores-per-socket %u\n",
            f->nx, f->pae, f->cores_per_socket);
    xg_info("apic: %d acpi: %d acpi_s4: %d acpi_s3: %d tsc_mode: %d hpet: %d\n",
            f->apic, f->acpi, f->acpi_s4, f->acpi_s3, f->tsc_mode, f->hpet);
    xg_info("viridian: %d, time_ref_count: %d, reference_tsc: %d\n",
            f->viridian, f->viridian_time_ref_count, f->viridian_reference_tsc);
    for (n = 0; n < f->vcpus; n++){
        xg_info("vcpu/%d/affinity:%s\n", n, (f->vcpu_affinity[n])?f->vcpu_affinity[n]:"unset");
    }
}

static void free_flags(struct flags *f)
{
    int n;
    for ( n = 0; n < f->vcpus; ++n )
        free(f->vcpu_affinity[n]);
    free(f->vcpu_affinity);
}

static void failwith_oss_xc(char *fct)
{
    char buf[80];
    const xc_error *error;

    error = xc_get_last_error(xch);
    if (error->code == XC_ERROR_NONE)
        snprintf(buf, 80, "%s: [%d] %s", fct, errno, strerror(errno));
    else
        snprintf(buf, 80, "%s: [%d] %s", fct, error->code, error->message);
    xc_clear_last_error(xch);
    xg_err("xenguest: %s\n", buf);
    exit(1);
}

extern struct xc_dom_image *xc_dom_allocate(xc_interface *xch, const char *cmdline, const char *features);

static void configure_vcpus(struct flags f){
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
                failwith_oss_xc("xc_cpumap_alloc");

            for (j=0; j<min; j++) {
                if (f.vcpu_affinity[i][j] == '1')
                    cpumap[j/8] |= 1 << (j&7);
            }
            r = xc_vcpu_setaffinity(xch, domid, i, cpumap, NULL,
                                    XEN_VCPUAFFINITY_HARD);
            free(cpumap);
            if (r) {
                failwith_oss_xc("xc_vcpu_setaffinity");
            }
        }
    }

    r = xc_sched_credit_domain_get(xch, domid, &sdom);
    /* This should only happen when a different scheduler is set */
    if (r) {
        xg_info("Failed to get credit scheduler parameters: scheduler not enabled?\n");
        return;
    }
    if (f.vcpu_weight != 0L) sdom.weight = f.vcpu_weight;
    if (f.vcpu_cap != 0L) sdom.cap = f.vcpu_cap;
    /* This shouldn't fail, if "get" above succeeds. This error is fatal
       to highlight the need to investigate further. */
    r = xc_sched_credit_domain_set(xch, domid, &sdom);
    if (r)
        failwith_oss_xc("xc_sched_credit_domain_set");
}

static uint64_t get_image_max_size(const char *type)
{
    char key[64];
    char *s;
    uint64_t max_size = 0;

    snprintf(key, sizeof(key), "/mh/limits/pv-%s-max_size", type);

    s = xs_read(xsh, XBT_NULL, key, NULL);
    if (s) {
        errno = 0;
        max_size = strtoull(s, NULL, 0);
        if (errno)
            max_size = 0;
        free(s);
    }
    return max_size ? max_size : XC_DOM_DECOMPRESS_MAX;
}

static void configure_tsc(struct flags f)
{
    int rc = xc_domain_set_tsc_info(xch, domid, f.tsc_mode, 0, 0, 0);

    if (rc)
        failwith_oss_xc("xc_domain_set_tsc_info");
}


int stub_xc_linux_build(int c_mem_max_mib, int mem_start_mib,
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
    get_flags(&f);

    xc_dom_loginit(xch);
    dom = xc_dom_allocate(xch, cmdline, features);
    if (!dom)
        failwith_oss_xc("xc_dom_allocate");

    /* The default image size limits are too large. */
    xc_dom_kernel_max_size(dom, get_image_max_size("kernel"));
    xc_dom_ramdisk_max_size(dom, get_image_max_size("ramdisk"));

    configure_vcpus(f);
    configure_tsc(f);

    r = xc_dom_linux_build(xch, dom, domid, mem_start_mib,
                           image_name, ramdisk_name, flags,
                           store_evtchn, store_mfn,
                           console_evtchn, console_mfn);
    if ( r )
        failwith_oss_xc("xc_dom_linux_build");

    r = construct_cpuid_policy(&f, false);
    if ( r )
        failwith_oss_xc("construct_cpuid_policy");

    r = xc_dom_gnttab_seed(xch, domid,
                           *console_mfn, *store_mfn,
                           console_domid, store_domid);
    if ( r )
        failwith_oss_xc("xc_dom_gnttab_seed");

    strncpy(protocol, xc_domain_get_native_protocol(xch, domid), 64);

    free_flags(&f);
    xc_dom_release(dom);

    return 0;
}

static void hvm_set_viridian_features(struct flags *f)
{
    uint64_t feature_mask = HVMPV_base_freq;

    xg_info("viridian base\n");

    if (f->viridian_time_ref_count) {
        xg_info("+ time_ref_count\n");
        feature_mask |= HVMPV_time_ref_count;
    }

    if (f->viridian_reference_tsc) {
        xg_info("+ viridian_reference_tsc\n");
        feature_mask |= HVMPV_reference_tsc;
    }

    xc_set_hvm_param(xch, domid, HVM_PARAM_VIRIDIAN, feature_mask);
}

static int hvm_build_set_params(int store_evtchn, unsigned long *store_mfn,
                                int console_evtchn, unsigned long *console_mfn,
                                struct flags f)
{
    struct hvm_info_table *va_hvm;
    uint8_t *va_map, sum;
    uint32_t i;
    int rc = 0;

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

    if (f.viridian)
        hvm_set_viridian_features(&f);

    xc_set_hvm_param(xch, domid, HVM_PARAM_STORE_EVTCHN, store_evtchn);
    xc_set_hvm_param(xch, domid, HVM_PARAM_HPET_ENABLED, f.hpet);
    xc_set_hvm_param(xch, domid, HVM_PARAM_NESTEDHVM, f.nested_hvm);
    xc_get_hvm_param(xch, domid, HVM_PARAM_CONSOLE_PFN, console_mfn);
    xc_set_hvm_param(xch, domid, HVM_PARAM_CONSOLE_EVTCHN, console_evtchn);
    xc_set_hvm_param(xch, domid, HVM_PARAM_TRIPLE_FAULT_REASON, SHUTDOWN_crash);

#ifdef HAVE_CORES_PER_SOCKET
    if ( f.cores_per_socket > 0 )
        rc = xc_domain_set_cores_per_socket(xch, domid, f.cores_per_socket);
#endif
    
    return rc;
}

static int get_rdm(uint16_t seg, uint8_t bus, uint8_t devfn,
                   unsigned int *nr_entries,
                   struct xen_reserved_device_memory **xrdm)
{
    int rc = 0, r;

    *nr_entries = 0;
    r = xc_reserved_device_memory_map(xch, 0, seg, bus, devfn,
                                      NULL, nr_entries);
    /* "0" means we have no any rdm entry. */
    if (!r)
        goto out;

    if (errno != ENOBUFS) {
        rc = -1;
        goto out;
    }
    *xrdm = malloc(sizeof(**xrdm) * (*nr_entries));
    if (!*xrdm)
        rc = -1;

    r = xc_reserved_device_memory_map(xch, 0, seg, bus, devfn,
                                      *xrdm, nr_entries);
    if (r)
        rc = -1;
out:
    return rc;
}

/* Copied from xen hypervisor itself */
const char * parse_pci_sbdf(char *s, unsigned int *seg_p,
                             unsigned int *bus_p, unsigned int *dev_p,
                             unsigned int *func_p)
{
    unsigned long seg = strtoul(s, &s, 16), bus, dev, func;

    if ( *s != ':' )
        return NULL;
    bus = strtoul(s + 1, &s, 16);
    if ( *s == ':' )
        dev = strtoul(s + 1, &s, 16);
    else
    {
        dev = bus;
        bus = seg;
        seg = 0;
    }
    if ( func_p )
    {
        if ( *s != '.' )
            return NULL;
        func = strtoul(s + 1, &s, 0);
    }
    else
        func = 0;

    if ( seg_p )
        *seg_p = seg;
    *bus_p = bus;
    *dev_p = dev;
    if ( func_p )
        *func_p = func;

    return s;
}

#define MAX_RMRR_DEVICES 16

int stub_xc_hvm_build_with_mem(uint64_t max_mem_mib, uint64_t max_start_mib,
                               const char *image)
{
    uint64_t lowmem_end, highmem_start, highmem_end, mmio_start;
    struct xc_hvm_build_args args = {
        .mem_size   = max_mem_mib   << 20,
        .mem_target = max_start_mib << 20,
        .mmio_size  = HVM_BELOW_4G_MMIO_LENGTH,
        .image_file_name = image,
    };
    unsigned int i, j, nr = 0;
    struct e820entry *e820;
    int rc;
    unsigned int nr_rdm_entries[MAX_RMRR_DEVICES] = {0};
    unsigned int nr_rmrr_devs = 0;
    struct xen_reserved_device_memory *xrdm[MAX_RMRR_DEVICES] = {0};
    unsigned long rmrr_overlapped_ram = 0;
    char *s;

    if ( pci_passthrough_sbdf_list )
    {
        s = strtok(pci_passthrough_sbdf_list,",");
        while ( s != NULL )
        {
            unsigned int seg, bus, device, func;
            xg_info("Getting RMRRs for device '%s'\n",s);
            if ( parse_pci_sbdf(s, &seg, &bus, &device, &func) )
            {
                if ( !get_rdm(seg, bus, (device << 3) + func,
                        &nr_rdm_entries[nr_rmrr_devs], &xrdm[nr_rmrr_devs]) )
                    nr_rmrr_devs++;
            }
            if ( nr_rmrr_devs == MAX_RMRR_DEVICES )
            {
                xg_err("Error: hit limit of %d RMRR devices for domain\n",
                            MAX_RMRR_DEVICES);
                exit(1);
            }
            s = strtok (NULL, ",");
        }
    }
    e820 = malloc(sizeof(*e820) * E820MAX);
    if (!e820)
	    return -ENOMEM;

    lowmem_end  = args.mem_size;
    highmem_end = highmem_start = 1ull << 32;
    mmio_start  = highmem_start - args.mmio_size;

    if ( lowmem_end > mmio_start )
    {
        highmem_end = (1ull << 32) + (lowmem_end - mmio_start);
        lowmem_end = mmio_start;
    }

    args.lowmem_end = lowmem_end;
    args.highmem_end = highmem_end;
    args.mmio_start = mmio_start;

    /* Leave low 1MB to HVMLoader... */
    e820[nr].addr = 0x100000u;
    e820[nr].size = args.lowmem_end - 0x100000u;
    e820[nr].type = E820_RAM;
    nr++;

    /* RDM mapping */
    for (i = 0; i < nr_rmrr_devs; i++)
    {
        for (j = 0; j < nr_rdm_entries[i]; j++)
        {
            e820[nr].addr = xrdm[i][j].start_pfn << XC_PAGE_SHIFT;
            e820[nr].size = xrdm[i][j].nr_pages << XC_PAGE_SHIFT;
            e820[nr].type = E820_RESERVED;
            xg_info("Adding RMRR 0x%lx size 0x%lx\n", e820[nr].addr, e820[nr].size);
            if ( e820[nr].addr < args.lowmem_end ) {
                rmrr_overlapped_ram += ( args.lowmem_end - e820[nr].addr );
                args.lowmem_end = e820[nr].addr;
            }
            nr++;
        }
    }
    e820[0].size -= rmrr_overlapped_ram;
    args.highmem_end += rmrr_overlapped_ram;
    args.mmio_size += rmrr_overlapped_ram;
    args.mmio_start -= rmrr_overlapped_ram;

    for (i = 0; i < nr_rmrr_devs; i++)
    {
        free(xrdm[i]);
    }

    if ( args.highmem_end > highmem_start )
    {
        e820[nr].addr = highmem_start;
        e820[nr].size = args.highmem_end - e820[nr].addr;
        e820[nr].type = E820_RAM;
        nr++;
    }

    rc = xc_hvm_build(xch, domid, &args);

    if (!rc)
        rc = xc_domain_set_memory_map(xch, domid, e820, nr);

    free(e820);

    return rc;
}

int stub_xc_hvm_build(int mem_max_mib, int mem_start_mib, const char *image_name,
                      int store_evtchn, int store_domid,
                      int console_evtchn, int console_domid,
                      unsigned long *store_mfn, unsigned long *console_mfn)
{
    int r;
    struct flags f;
    get_flags(&f);

    configure_vcpus(f);
    configure_tsc(f);

    r = stub_xc_hvm_build_with_mem(mem_max_mib, mem_start_mib, image_name);
    if ( r )
        failwith_oss_xc("hvm_build");

    r = hvm_build_set_params(store_evtchn, store_mfn,
                             console_evtchn, console_mfn, f);
    if ( r )
        failwith_oss_xc("hvm_build_params");

    r = construct_cpuid_policy(&f, true);
    if ( r )
        failwith_oss_xc("construct_cpuid_policy");

    r = xc_dom_gnttab_hvm_seed(xch, domid, *console_mfn, *store_mfn,
                               console_domid, store_domid);
    if ( r )
        failwith_oss_xc("xc_dom_gnttab_hvm_seed");

    free_flags(&f);

    return 0;
}

static int switch_qemu_logdirty(int _domid, unsigned enable, void *_data)
{
    char buf[64], *reply = NULL;
    static const char cmd_enable[] = "enable";
    static const char cmd_disable[] = "disable";
    const char *cmd = enable ? cmd_enable : cmd_disable;
    struct timeval start, now;
    uint64_t timeout_us = 15 * 1000 * 1000; /* 15 seconds, in microseconds. */
    bool rc = 0;

    snprintf(buf, sizeof buf, "/local/domain/0/device-model/%u/logdirty/cmd", domid);
    if ( !xs_write(xsh, XBT_NULL, buf, cmd, strlen(cmd)) )
    {
        int saved_errno = errno;

        xg_err("Failed to write logdirty '%s' command to xenstore\n", cmd);
        errno = saved_errno;
        return 1;
    }

    xg_info("Waiting for qemu to confirm logdirty '%s'\n", cmd);

    snprintf(buf, sizeof buf, "/local/domain/0/device-model/%u/logdirty/cmd", domid);
    gettimeofday(&start, NULL);
    do
    {
        usleep(100000); /* Wait a short while for Qemu to reply. */

        reply = xs_read(xsh, XBT_NULL, buf, NULL);

        if ( reply )
        {
            rc = strcmp(cmd, reply) == 0;
            free(reply);
        }

        if ( !rc )
            gettimeofday(&now, NULL);
    } while ( !rc && (tv_delta_us(&now, &start) < timeout_us) );

    if ( !rc )
    {
        xg_err("Timeout waiting for qemu to acknowledge logdirty '%s'\n", cmd);
        errno = ETIMEDOUT;
    }
    else
        xg_info("  got reply\n");

    return !rc;
}

#define GENERATION_ID_ADDRESS "hvmloader/generation-id-address"

int stub_xc_domain_save(int fd, int max_iters, int max_factors,
                        int flags, int hvm)
{
    int r;
    struct save_callbacks callbacks =
        {
            .suspend = suspend_callback,
            .switch_qemu_logdirty = switch_qemu_logdirty,
            .data = NULL,
        };

    r = xc_domain_save(xch, fd, domid,
                       max_iters, max_factors,
                       flags, &callbacks, hvm);
    if (r)
        failwith_oss_xc("xc_domain_save");

    return 0;
}

/* this is the slow version of resume for uncooperative domain,
 * the fast version is available in close source xc */
int stub_xc_domain_resume_slow(void)
{
    int r;

    /* hard code fast to 0, we only want to expose the slow version here */
    r = xc_domain_resume(xch, domid, 0);
    if (r)
        failwith_oss_xc("xc_domain_resume");
    return 0;
}

static int set_genid(void)
{
    uint64_t paddr = 0;
    void *vaddr;
    char *genid_val_str;
    char *end;
    uint64_t genid[2];
    int rc = -1;

    xc_get_hvm_param(xch, domid, HVM_PARAM_VM_GENERATION_ID_ADDR, &paddr);
    if (paddr == 0)
        return 0;

    genid_val_str = xenstore_gets("platform/generation-id");
    if ( !genid_val_str )
        return 0;

    errno = 0;
    genid[0] = strtoull(genid_val_str, &end, 0);
    genid[1] = 0;
    if ( end && end[0] == ':' )
        genid[1] = strtoull(end+1, NULL, 0);

    if ( errno )
    {
        xg_err("strtoull of '%s' failed: %s\n", genid_val_str, strerror(errno));
        goto out;
    }
    else if ( genid[0] == 0 || genid[1] == 0 )
    {
        xg_err("'%s' is not a valid generation id\n", genid_val_str);
        goto out;
    }

    vaddr = xc_map_foreign_range(xch, domid, XC_PAGE_SIZE,
                                 PROT_READ | PROT_WRITE,
                                 paddr >> XC_PAGE_SHIFT);
    if (vaddr == NULL) {
        xg_err("Failed to map VM generation ID page: %s\n", strerror(errno));
        goto out;
    }
    memcpy(vaddr + (paddr & ~XC_PAGE_MASK), genid, 2 * sizeof(*genid));
    munmap(vaddr, XC_PAGE_SIZE);

    /*
     * FIXME: Inject ACPI Notify event.
     */

    xg_info("Wrote generation ID %"PRId64":%"PRId64" at 0x%"PRIx64"\n",
         genid[0], genid[1], paddr);
    rc = 0;

 out:
    free(genid_val_str);
    return rc;
}

int stub_xc_domain_restore(int fd, int store_evtchn, int console_evtchn,
                           int hvm,
                           unsigned long *store_mfn, unsigned long *console_mfn)
{
    int r = 0;
    struct flags f;

    get_flags(&f);

    if ( hvm )
    {
        /*
         * We have to do this even in the domain restore case as XenServers
         * prior to 6.0.2 did not create a viridian save record.
         */
        if (f.viridian)
            hvm_set_viridian_features(&f);

        xc_set_hvm_param(xch, domid, HVM_PARAM_HPET_ENABLED, f.hpet);
#ifdef HAVE_CORES_PER_SOCKET
        if ( f.cores_per_socket > 0 )
            r = xc_domain_set_cores_per_socket(xch, domid, f.cores_per_socket);
#endif
        if ( r )
            failwith_oss_xc("xc_domain_set_cores_per_socket");
    }

    configure_vcpus(f);

    r = xc_domain_restore(xch, fd, domid,
                          store_evtchn, store_mfn, 0,
                          console_evtchn, console_mfn, 0,
                          hvm, f.pae, 0, 0, NULL);
    if ( r )
        failwith_oss_xc("xc_domain_restore");
    /*
     * The legacy -> migration v2 code in XenServer 6.5 didn't combine the
     * out-of-band HVM_PARAM_PAE_ENABLED into the converted stream, and
     * xenguest didn't set it, as the v2 restore code was expected to.
     *
     * This causes xc_cpuid_apply_policy() to hide the PAE bit from the domain
     * cpuid policy, which went unnoticed (and without incident, despite being
     * a guest-visible change) until Xen-4.5 became stricter with its checks
     * for when a guest writes to %cr4.
     *
     * The correct value is still available out-of-band, so clobber the result
     * from the stream, in case the stream is from XenServer 6.5 and is a VM
     * which hasn't rebooted and has a bad HVM PARAM in the v2 stream.
     */
    if ( hvm )
        xc_set_hvm_param(xch, domid, HVM_PARAM_PAE_ENABLED, f.pae);

    r = construct_cpuid_policy(&f, hvm);
    if ( r )
        failwith_oss_xc("construct_cpuid_policy");

    free_flags(&f);

    if ( hvm )
    {
        r = set_genid();
        if (r)
            exit(1);
    }

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
