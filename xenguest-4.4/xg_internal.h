#ifndef __XG_INTERNAL_H__
#define __XG_INTERNAL_H__

void err(const char *msg, ...);
void info(const char *msg, ...);

typedef struct xs_handle xs_handle;

int stub_xc_linux_build(xc_interface *xch, xs_handle *xsh, int domid,
                        int c_mem_max_mib, int mem_start_mib,
                        const char *image_name, const char *ramdisk_name,
                        const char *cmdline, const char *features,
                        int flags, int store_evtchn, int store_domid,
                        int console_evtchn, int console_domid,
                        unsigned long *store_mfn, unsigned long *console_mfn,
                        char *protocol);
int stub_xc_hvm_build(xc_interface *xch, xs_handle *xsh, int domid,
                      int mem_max_mib, int mem_start_mib, const char *image_name,
                      int store_evtchn, int store_domid, int console_evtchn,
                      int console_domid, unsigned long *store_mfn, unsigned long *console_mfn);
int stub_xc_domain_save(xc_interface *xch, xs_handle *xsh, int fd, int domid,
                        int max_iters, int max_factors,
                        int flags, int hvm);
int stub_xc_domain_restore(xc_interface *xch, xs_handle *xsh, int fd, int domid,
                           int store_evtchn, int console_evtchn,
                           int hvm,
                           unsigned long *store_mfn, unsigned long *console_mfn);
int stub_xc_domain_resume_slow(xc_interface *xch, xs_handle *xsh, int domid);

int suspend_callback(int domid);

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
