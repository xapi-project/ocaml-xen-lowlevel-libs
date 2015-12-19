#ifndef __XG_INTERNAL_H__
#define __XG_INTERNAL_H__

#include <inttypes.h>
#include <sys/time.h>

#define __printf(f, v) __attribute__((format(__printf__, f, v)))

enum xenguest_mode {
    XG_MODE_SAVE,
    XG_MODE_HVM_SAVE,
    XG_MODE_RESTORE,
    XG_MODE_HVM_RESTORE,
    XG_MODE_RESUME_SLOW,
    XG_MODE_LINUX_BUILD,
    XG_MODE_HVM_BUILD,
    XG_MODE_TEST,
    XG_MODE__END__,
};

void xg_err(const char *msg, ...) __printf(1, 2);
void xg_info(const char *msg, ...) __printf(1, 2);

typedef struct xs_handle xs_handle;

extern xc_interface *xch;
extern xs_handle *xsh;
extern int domid;

/* Read and write /local/domain/$domid/ relative paths. */
char *xenstore_getsv(const char *fmt, va_list ap);
char *xenstore_gets(const char *fmt, ...)  __printf(1, 2);
uint64_t xenstore_get(const char *fmt, ...) __printf(1, 2);
int xenstore_putsv(const char *key, const char *fmt, ...) __printf(2, 3);
int xenstore_puts(const char *key, const char *val);

int stub_xc_linux_build(int c_mem_max_mib, int mem_start_mib,
                        const char *image_name, const char *ramdisk_name,
                        const char *cmdline, const char *features,
                        int flags, int store_evtchn, int store_domid,
                        int console_evtchn, int console_domid,
                        unsigned long *store_mfn, unsigned long *console_mfn,
                        char *protocol);
int stub_xc_hvm_build(int mem_max_mib, int mem_start_mib, const char *image_name,
                      int store_evtchn, int store_domid, int console_evtchn,
                      int console_domid, unsigned long *store_mfn, unsigned long *console_mfn);
int stub_xc_domain_save(int fd, int max_iters, int max_factors,
                        int flags, int hvm);
int stub_xc_domain_restore(int fd, int store_evtchn, int console_evtchn,
                           int hvm,
                           unsigned long *store_mfn, unsigned long *console_mfn);
int stub_xc_domain_resume_slow(void);

int suspend_callback(void *data);

void setup_legacy_conversion(int opt_fd, enum xenguest_mode mode);
void cleanup_legacy_conversion(void);

extern char *xs_domain_path;
extern char *pci_passthrough_sbdf_list;

/* Calcluate the difference between two timevals, in microseconds. */
static inline uint64_t tv_delta_us(const struct timeval *new,
                                   const struct timeval *old)
{
    return (((new->tv_sec - old->tv_sec)*1000000) +
            (new->tv_usec - old->tv_usec));
}

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
