#include <getopt.h>
#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>

#include <xenctrl.h>
#include <xenguest.h>
#include <xenstore.h>

#include "xg_internal.h"

/*
 * Xapi uses a strange protocol to communicate which xenguest, which seems to
 * be a relic from the xend days.
 *
 * For all domain functions, Xapi expects on the controloutfd:
 *
 *   result:<xenstore mfn> <console mfn>[ <PV ABI>]\n
 *
 * with the xenstore and console mfn in decimal and a PV ABI for PV domains
 * only; HVM domains only require the two mfns.  This information is only
 * relevent when constructing and restoring domains, but must be present for
 * suspend as well, with 0 for both MFNs and no ABI.
 *
 * In addition for suspend only, Xapi expects to see the string "suspend:\n"
 * written to the controloutfd, and expects xenguest to wait until it
 * successfully reads a line from controlinfd.
 */

enum xenguest_opts {
    XG_OPT_MODE, /* choice */
    XG_OPT_CONTROLINFD, /* int */
    XG_OPT_CONTROLOUTFD, /* int */
    XG_OPT_DEBUGLOG, /* str */
    XG_OPT_FAKE, /* bool */
    XG_OPT_FD, /* int */
    XG_OPT_IMAGE, /* str */
    XG_OPT_CMDLINE, /* str */
    XG_OPT_RAMDISK, /* str */
    XG_OPT_DOMID, /* int */
    XG_OPT_LIVE, /* bool */
    XG_OPT_DEBUG, /* bool */
    XG_OPT_STORE_PORT, /* str */
    XG_OPT_STORE_DOMID, /* str */
    XG_OPT_CONSOLE_PORT, /* str */
    XG_OPT_CONSOLE_DOMID, /* str */
    XG_OPT_FEATURES, /* str */
    XG_OPT_FLAGS, /* int */
    XG_OPT_MEM_MAX_MIB, /* int */
    XG_OPT_MEM_START_MIB, /* int */
    XG_OPT_FORK, /* bool */
    XG_OPT_NO_INC_GENID, /* bool */
    XG_OPT_SUPPORTS, /* str */
    XG_OPT_PCI_PASSTHROUGH, /* str */
};

static int opt_mode = -1;
static int opt_controlinfd = -1;
static int opt_controloutfd = -1;
static FILE *opt_debugfile = NULL;
static int opt_fd = -1;
static const char *opt_image = NULL;
static const char *opt_cmdline = NULL;
static const char *opt_ramdisk = NULL;
static int opt_store_port = -1;
static int opt_store_domid = 0;
static int opt_console_port = -1;
static int opt_console_domid = 0;
static const char *opt_features = NULL;
static int opt_flags = 0;
static int opt_mem_max_mib = -1;
static int opt_mem_start_mib = -1;

static const char *xg_mode_names[] = {
    [XG_MODE_SAVE] = "save",
    [XG_MODE_HVM_SAVE] = "hvm_save",
    [XG_MODE_RESTORE] = "restore",
    [XG_MODE_HVM_RESTORE] = "hvm_restore",
    [XG_MODE_RESUME_SLOW] = "resume_slow",
    [XG_MODE_LINUX_BUILD] = "linux_build",
    [XG_MODE_HVM_BUILD] = "hvm_build",
    [XG_MODE_TEST] = "test",
};

xc_interface *xch = NULL;
xs_handle *xsh = NULL;
int domid = -1;

void xg_err(const char *msg, ...)
{
    char *buf = NULL;
    va_list args;
    int rc;

    va_start(args, msg);
    rc = vasprintf(&buf, msg, args);
    va_end(args);

    if ( rc != -1 )
    {
        if ( opt_debugfile )
            fputs(buf, opt_debugfile);
        fputs(buf, stderr);
        syslog(LOG_ERR|LOG_DAEMON, "%s", buf);

        if ( opt_controloutfd != -1 )
        {
            char reply[] = "error:";

            write(opt_controloutfd, reply, strlen(reply));
            write(opt_controloutfd, buf, rc);
        }
    }

    free(buf);
}

void xg_info(const char *msg, ...)
{
    char *buf = NULL;
    va_list args;
    int rc;

    va_start(args, msg);
    rc = vasprintf(&buf, msg, args);
    va_end(args);

    if ( rc != -1 )
    {
        if ( opt_debugfile )
            fputs(buf, opt_debugfile);
        syslog(LOG_INFO|LOG_DAEMON, "%s", buf);
    }

    free(buf);
}

static void logfn(struct xentoollog_logger *logger,
                  xentoollog_level level,
                  int errnoval,
                  const char *context,
                  const char *format,
                  va_list al)
{
    char *buf = NULL;

    if ( level == XTL_DEBUG && !(opt_flags & XCFLAGS_DEBUG) )
        return;

    if ( vasprintf(&buf, format, al) != -1 )
        xg_info("%s: %s: %s\n", context, xtl_level_to_string(level), buf);

    free(buf);
}

static void progressfn(struct xentoollog_logger *logger,
                       const char *context, const char *doing_what,
                       int percent, unsigned long done, unsigned long total)
{
    static struct timeval lasttime = { 0 };
    struct timeval curtime;

    gettimeofday(&curtime, NULL);

    if ( (done == 0) || (done == total) ||
         (tv_delta_us(&curtime, &lasttime) > (5UL * 1000 * 1000)) )
    {
        if ( done == 0 && total == 0 )
            xg_info("progress: %s\n", doing_what);
        else
            xg_info("progress: %s: %ld of %ld (%d%%)\n",
                 doing_what, done, total, percent);

        lasttime = curtime;
    }
}

static int parse_mode(const char *mode)
{
    int i;

    for (i = 0; i < XG_MODE__END__; i++) {
        if (strcmp(mode, xg_mode_names[i]) == 0)
            return i;
    }
    xg_err("xenguest: unrecognized mode '%s'\n", mode);
    exit(1);
}

static int parse_int(const char *str)
{
    char *end;
    int result;

    result = strtol(str, &end, 10);

    if (*end != '\0') {
        xg_err("xenguest: '%s' is not a valid integer\n", str);
        exit(1);
    }

    return result;
}

static void parse_options(int argc, char *const argv[])
{
    static const struct option opts[] = {
        { "mode", required_argument, NULL, XG_OPT_MODE, },
        { "controlinfd", required_argument, NULL, XG_OPT_CONTROLINFD, },
        { "controloutfd", required_argument, NULL, XG_OPT_CONTROLOUTFD, },
        { "debuglog", required_argument, NULL, XG_OPT_DEBUGLOG, },
        { "fake", no_argument, NULL, XG_OPT_FAKE, },

        { "fd", required_argument, NULL, XG_OPT_FD, },
        { "image", required_argument, NULL, XG_OPT_IMAGE, },
        { "cmdline", required_argument, NULL, XG_OPT_CMDLINE, },
        { "ramdisk", required_argument, NULL, XG_OPT_RAMDISK, },
        { "domid", required_argument, NULL, XG_OPT_DOMID, },
        { "live", no_argument, NULL, XG_OPT_LIVE, },
        { "debug", no_argument, NULL, XG_OPT_DEBUG, },
        { "store_port", required_argument, NULL, XG_OPT_STORE_PORT, },
        { "store_domid", required_argument, NULL, XG_OPT_STORE_DOMID, },
        { "console_port", required_argument, NULL, XG_OPT_CONSOLE_PORT, },
        { "console_domid", required_argument, NULL, XG_OPT_CONSOLE_DOMID, },
        { "features", required_argument, NULL, XG_OPT_FEATURES, },
        { "flags", required_argument, NULL, XG_OPT_FLAGS, },
        { "mem_max_mib", required_argument, NULL, XG_OPT_MEM_MAX_MIB, },
        { "mem_start_mib", required_argument, NULL, XG_OPT_MEM_START_MIB, },
        { "fork", no_argument, NULL, XG_OPT_FORK, },
        { "no_incr_generationid", no_argument, NULL, XG_OPT_NO_INC_GENID, },
        { "supports", required_argument, NULL, XG_OPT_SUPPORTS, },
        { "pci_passthrough", required_argument, NULL, XG_OPT_PCI_PASSTHROUGH, },
        { NULL },
    };

    int c;

    for(;;) {
        int option_index = 0;

        c = getopt_long_only(argc, argv, "", opts, &option_index);

        switch (c) {
        case -1:
            return;

        case XG_OPT_MODE:
            opt_mode = parse_mode(optarg);
            break;

        case XG_OPT_CONTROLINFD:
            opt_controlinfd = parse_int(optarg);
            break;

        case XG_OPT_CONTROLOUTFD:
            opt_controloutfd = parse_int(optarg);
            break;

        case XG_OPT_DEBUGLOG:
            if ( opt_debugfile && fclose(opt_debugfile) )
            {
                xg_err("Unable to close existing debug file: %d %s\n",
                    errno, strerror(errno));
                exit(1);
            }

            opt_debugfile = fopen(optarg, "a");
            if ( !opt_debugfile )
            {
                xg_err("Unable to open debug file '%s': %d %s\n",
                    optarg, errno, strerror(errno));
                exit(1);
            }
            break;

        case XG_OPT_FD:
            opt_fd = parse_int(optarg);
            break;

        case XG_OPT_IMAGE:
            opt_image = optarg;
            break;

        case XG_OPT_CMDLINE:
            opt_cmdline = optarg;
            break;

        case XG_OPT_RAMDISK:
            opt_ramdisk = optarg;
            break;

        case XG_OPT_DOMID:
            domid = parse_int(optarg);
            break;

        case XG_OPT_LIVE:
            opt_flags |= XCFLAGS_LIVE;
            break;

        case XG_OPT_DEBUG:
            opt_flags |= XCFLAGS_DEBUG;
            break;

        case XG_OPT_STORE_PORT:
            opt_store_port = parse_int(optarg);
            break;

        case XG_OPT_STORE_DOMID:
            opt_store_domid = parse_int(optarg);
            break;

        case XG_OPT_CONSOLE_PORT:
            opt_console_port = parse_int(optarg);
            break;

        case XG_OPT_CONSOLE_DOMID:
            opt_console_domid = parse_int(optarg);
            break;

        case XG_OPT_FEATURES:
            opt_features = optarg;
            break;

        case XG_OPT_FLAGS:
            opt_flags = parse_int(optarg);
            break;

        case XG_OPT_MEM_MAX_MIB:
            opt_mem_max_mib = parse_int(optarg);
            break;

        case XG_OPT_MEM_START_MIB:
            opt_mem_start_mib = parse_int(optarg);
            break;

        case XG_OPT_SUPPORTS:
            if ( !strcmp("migration-v2", optarg) )
            {
                if ( getenv("XG_MIGRATION_V2") )
                {
                    printf("true\n");
                    exit(0);
                }
                else
                {
                    printf("false\n");
                    exit(0);
                }
            }
            else
            {
                printf("false\n");
                exit(0);
            }
            break;

        case XG_OPT_PCI_PASSTHROUGH:
            pci_passthrough_sbdf_list = optarg;
            break;

        case XG_OPT_FAKE:
        case XG_OPT_FORK:
        case XG_OPT_NO_INC_GENID:
            /* ignored */
            break;

        default:
            xg_err("xenguest: invalid command line '%s'\n", argv[optind - 1]);
            exit(1);
        }
    }
}

static void set_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);

    if ( flags == -1 )
    {
        xg_err("Failed fcntl(F_GETFD) on fd %d: %d, %s\n",
               fd, errno, strerror(errno));
        exit(1);
    }

    if ( fcntl(fd, flags | FD_CLOEXEC) == -1 )
    {
        xg_err("Failed to set FD_CLOEXEC on fd %d: %d %s\n",
            fd, errno, strerror(errno));
        exit(1);
    }
}

static void write_status(unsigned long store_mfn, unsigned long console_mfn,
                         const char *protocol)
{
    if ( opt_controloutfd != -1 )
    {
        char buf[64];
        size_t len;

        if (protocol)
            len = snprintf(buf, sizeof(buf), "result:%lu %lu %s\n", store_mfn, console_mfn, protocol);
        else
            len = snprintf(buf, sizeof(buf), "result:%lu %lu\n", store_mfn, console_mfn);
        write(opt_controloutfd, buf, len);
        xg_info("Writing to control: '%s'\n", buf);
    }
    else
        xg_err("No control fd to write success to\n");
}

static void do_save(bool is_hvm)
{
    if (domid == -1 || opt_fd == -1) {
        xg_err("xenguest: missing command line options\n");
        exit(1);
    }

    stub_xc_domain_save(opt_fd, 0, 0, opt_flags, is_hvm);
    write_status(0, 0, NULL);
}

static void do_restore(bool is_hvm)
{
    unsigned long store_mfn = 0, console_mfn = 0;

    if (domid == -1 || opt_fd == -1
        || opt_store_port == -1 || opt_console_port == -1) {
        xg_err("xenguest: missing command line options\n");
        exit(1);
    }

    stub_xc_domain_restore(opt_fd, opt_store_port, opt_console_port, is_hvm,
                           &store_mfn, &console_mfn);

    write_status(store_mfn, console_mfn, NULL);
}

static void do_resume(void)
{
    if (domid == -1) {
        xg_err("xenguest: missing command line options\n");
        exit(1);
    }

    stub_xc_domain_resume_slow();
    write_status(0, 0, NULL);
}

int suspend_callback(void *data)
{
    static const char suspend_message[] = "suspend:\n";

    write(opt_controloutfd, suspend_message, sizeof(suspend_message)-1);

    /* Read one line from control fd. */
    for (;;) {
        char buf[8];
        ssize_t len, i;

        len = read(opt_controlinfd, buf, sizeof(buf));
        if (len < 0 && errno == EINTR)
            continue;
        if (len < 0) {
            xg_err("xenguest: read from control FD failed: %s\n", strerror(errno));
            return 0;
        }
        if (len == 0) {
            xg_err("xenguest: unexpected EOF on control FD\n");
            return 0;
        }
        for ( i = 0; i < len; ++i )
            if (buf[i] == '\n')
                return 1;
    }
}

static void do_linux_build(void)
{
    unsigned long store_mfn = 0, console_mfn = 0;
    char protocol[64];

    if (domid == -1 || opt_mem_max_mib == -1 || opt_mem_start_mib == -1
        || !opt_image || !opt_ramdisk || !opt_cmdline
        || !opt_features || opt_flags == -1
        || opt_store_port == -1 || opt_store_domid == -1
        || opt_console_port == -1 || opt_console_domid == -1) {
        xg_err("xenguest: missing command line options\n");
        exit(1);
    }

    stub_xc_linux_build(opt_mem_max_mib, opt_mem_start_mib,
                        opt_image, opt_ramdisk,
                        opt_cmdline, opt_features, opt_flags,
                        opt_store_port, opt_store_domid,
                        opt_console_port, opt_console_domid,
                        &store_mfn, &console_mfn, protocol);
    write_status(store_mfn, console_mfn, protocol);
}

static void do_hvm_build(void)
{
    unsigned long store_mfn = 0, console_mfn = 0;

    if (domid == -1 || opt_mem_max_mib == -1 || opt_mem_start_mib == -1
        || !opt_image || opt_store_port == -1 || opt_store_domid == -1
        || opt_console_port == -1 || opt_console_domid == -1) {
        xg_err("xenguest: missing command line options\n");
        exit(1);
    }

    stub_xc_hvm_build(opt_mem_max_mib, opt_mem_start_mib,
                      opt_image, opt_store_port, opt_store_domid,
                      opt_console_port, opt_console_domid, &store_mfn,
                      &console_mfn);
    write_status(store_mfn, console_mfn, NULL);
}

static void do_test(void)
{
    xg_err("xenguest: test mode not supported\n");
    exit(1);
}

int main(int argc, char * const argv[])
{
    static char ident[32];
    char *cmdline = NULL;
    static xentoollog_logger logger = { logfn, progressfn, NULL };

    /* Force migration v2 all the time. */
    setenv("XG_MIGRATION_V2", "", 1);

    {   /* Conjoin the command line into a single string for logging */
        size_t sum, s;
        int i;
        char *ptr;

        sum = argc-1; /* Account for spaces and null */
        for ( i = 1; i < argc; ++i )
            sum += strlen(argv[i]);

        ptr = cmdline = malloc(sum);

        if ( !cmdline )
        {
            fprintf(stderr, "Out of Memory\n");
            exit(1);
        }

        for ( i = 1; i < argc; ++i )
        {
            s = strlen(argv[i]);
            memcpy(ptr, argv[i], s);
            ptr[s] = ' ';
            ptr = &ptr[s+1];
        }
        ptr[-1] = 0;
    }

    parse_options(argc, argv);

    /* Set up syslog with the domid and action in the ident string */
    if ( domid >= 0 )
    {
        const char *suffix;

        switch ( opt_mode )
        {
        case XG_MODE_SAVE:
        case XG_MODE_HVM_SAVE:
            suffix = "-save";
            break;

        case XG_MODE_RESTORE:
        case XG_MODE_HVM_RESTORE:
            suffix = "-restore";
            break;

        case XG_MODE_RESUME_SLOW:
            suffix = "-resume";
            break;

        case XG_MODE_LINUX_BUILD:
        case XG_MODE_HVM_BUILD:
            suffix = "-build";
            break;

        default:
            suffix = "";
            break;
        }

        snprintf(ident, sizeof ident, "xenguest-%d%s", domid, suffix);
    }
    else
        strncpy(ident, "xenguest", sizeof ident);

    openlog(ident, LOG_NDELAY | LOG_PID, LOG_DAEMON);

    xg_info("Command line: %s\n", cmdline);
    free(cmdline);

    set_cloexec(opt_controlinfd);
    set_cloexec(opt_controloutfd);

    xch = xc_interface_open(&logger, &logger, 0);
    if ( !xch ) {
        xg_err("xenguest: Failed to open xc interface\n");
        exit(1);
    }

    xsh = xs_open(0);
    if ( !xsh ) {
        xg_err("xenguest: Failed to open xenstore interface\n");
        exit(1);
    }

    if ( domid > 0 )
    {
        xs_domain_path = xs_get_domain_path(xsh, domid);

        if ( !xs_domain_path )
        {
            xg_err("Failed to obtain XenStore domain path\n");
            exit(1);
        }
    }

    switch (opt_mode) {
    case -1:
        xg_err("xenguest: no `-mode' option specified\n");
        exit(1);

    case XG_MODE_SAVE:
    case XG_MODE_HVM_SAVE:
        do_save(opt_mode == XG_MODE_HVM_SAVE);
        break;

    case XG_MODE_RESTORE:
    case XG_MODE_HVM_RESTORE:
        do_restore(opt_mode == XG_MODE_HVM_RESTORE);
        break;

    case XG_MODE_RESUME_SLOW:
        do_resume();
        break;

    case XG_MODE_LINUX_BUILD:
        do_linux_build();
        break;

    case XG_MODE_HVM_BUILD:
        do_hvm_build();
        break;

    case XG_MODE_TEST:
        do_test();
        break;
    }

    free(xs_domain_path);

    xs_close(xsh);
    xc_interface_close(xch);

    xg_info("All done\n");
    if ( opt_debugfile )
        fclose(opt_debugfile);
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
