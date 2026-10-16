/* NanoSync v0.0.0 - developed by tenspav */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include "ns_host.h"
#include "ns_rd.h"
static int g_sdk = -1;
static int sdk_on(void) { if (g_sdk < 0) g_sdk = getenv("NS_SDK") ? 1 : 0; return g_sdk; }
static uint32_t envu(const char *n, uint32_t d) { const char *v = getenv(n); return v ? (uint32_t)strtoul(v, 0, 10) : d; }
static ns_iodiag g_dg; static uint32_t g_roff;
char g_root[512]; long g_op, g_crash_at = -1; int g_crash_mode; jmp_buf g_crash_jmp; int g_battery = 80;
uint32_t g_heap_free = 3800u << 10, g_heap_largest = 2000u << 10, g_unix = 1760000000u; long g_alloc_now, g_alloc_peak; int g_fail_write_at, g_corrupt_at; static int g_nwrite, g_nwrite2;
static int rfd = -1, wfd = -1; static int g_wr_calls, g_w1_ok; static DIR *dd;
static int fake_fobj_read(void *o, uint32_t n, void *buf, uint32_t *got);
static void P(char *o, const char *path) { snprintf(o, 1024, "%s%s", g_root, path); }
void host_close_all(void) { if (rfd >= 0) close(rfd); if (wfd >= 0) close(wfd); rfd = wfd = -1; if (dd) closedir(dd); dd = 0; }
static void crash_before(void) { g_op++; if (g_crash_at > 0 && g_op == g_crash_at && g_crash_mode == 0) { host_close_all(); longjmp(g_crash_jmp, 1); } }
static void crash_after(void) { if (g_crash_at > 0 && g_op == g_crash_at && g_crash_mode == 1) { host_close_all(); longjmp(g_crash_jmp, 1); } }
static char g_wr_path[32][1024]; static long g_wr_old[32]; static int g_wr_n;
static int h_exists(const char *p) { char q[1024]; struct stat st; P(q, p); return stat(q, &st) == 0; }
static int32_t h_fsize(const char *p) { char q[1024]; struct stat st; int k; if (sdk_on() && envu("NS_SDK_FSIZESTALE", 0)) for (k = 0; k < g_wr_n; k++) if (!strcmp(g_wr_path[k], p) && g_wr_old[k] >= 0) return (int32_t)g_wr_old[k]; P(q, p); if (stat(q, &st) || S_ISDIR(st.st_mode)) return -1; return (int32_t)st.st_size; }
static int h_is_dir(const char *p) { char q[1024]; struct stat st; P(q, p); return stat(q, &st) == 0 && S_ISDIR(st.st_mode); }
static int h_mkdir(const char *p) { char q[1024]; int r; crash_before(); P(q, p); r = mkdir(q, 0755); if (r && errno == EEXIST) r = 0; crash_after(); return r; }
static int h_remove(const char *p) { char q[1024]; int r; size_t l = strlen(p); crash_before(); P(q, p);
    if (sdk_on() && envu("NS_SDK_RMFAIL", 0) && strstr(p, "/iPod_Control/iTunes/") && !(l > 4 && !strcmp(p + l - 4, ".bak"))) r = -1; else r = unlink(q);
    crash_after(); return r; }
static uint32_t h_read_all(const char *p, void *buf, uint32_t max) { char q[1024]; int fd; ssize_t n; P(q, p); fd = open(q, O_RDONLY); if (fd < 0) return 0; if (sdk_on()) { uint32_t cap = envu("NS_SDK_READMAX", 0); if (cap && max > cap) max = cap; }
    n = read(fd, buf, max); close(fd); return n < 0 ? 0 : (uint32_t)n; }
static int h_r_open(const char *p) { char q[1024]; P(q, p); if (rfd >= 0) { fprintf(stderr, "HOST: reader already open\n"); abort(); } rfd = open(q, O_RDONLY); g_roff = 0; g_dg.rc = 1; g_dg.req = g_dg.got = g_dg.calls = 0; return rfd < 0 ? -1 : 0; }
static int32_t h_r_read(void *b, uint32_t n) { ssize_t r; if (sdk_on()) return ns_seq_read(fake_fobj_read, 0, b, n, &g_dg); uint32_t have = 0; while (have < n) { r = read(rfd, (char *)b + have, n - have); if (r < 0) return -1; if (r == 0) break; have += (uint32_t)r; } return (int32_t)have; }
static int fake_fobj_read(void *o, uint32_t n, void *buf, uint32_t *got) {
    uint64_t *g64 = (uint64_t *)(void *)got; uint32_t maxgot = envu("NS_SDK_MAXGOT", 0), errat = envu("NS_SDK_ERRAT", 0); ssize_t r; (void)o;
    *g64 = 0;
    { uint32_t bz = envu("NS_SDK_BIGZERO", 0); if (bz && g_roff == 0 && n > bz) return 0; }
    if (errat && g_roff >= errat) return 9;
    if (maxgot && n > maxgot) n = maxgot;
    r = read(rfd, buf, n);
    if (r < 0) return 5;
    if (r == 0) return envu("NS_SDK_EOFRC", 0) ? 7 : 0;
    g_roff += (uint32_t)r; *g64 = (uint64_t)r; return 0;
}
static void h_io_diag(ns_iodiag *o) { *o = g_dg; }
static void h_r_close(void) { if (rfd >= 0) close(rfd); rfd = -1; }
static int h_w_open(const char *p) { char q[1024]; crash_before(); P(q, p); if (wfd >= 0) { fprintf(stderr, "HOST: writer already open\n"); abort(); } if (sdk_on() && g_wr_n < 32) { struct stat st0; snprintf(g_wr_path[g_wr_n], 1024, "%s", p); g_wr_old[g_wr_n] = stat(q, &st0) == 0 ? (long)st0.st_size : -1; g_wr_n++; }
    g_wr_calls = 0; g_w1_ok = envu("NS_SDK_WRITE1ONLY", 0) == 1 || (envu("NS_SDK_WRITE1ONLY", 0) == 2 && strstr(p, "/Music/")); wfd = open(q, O_WRONLY | O_CREAT | ((sdk_on() && envu("NS_SDK_NOTRUNC", 0)) ? 0 : O_TRUNC), 0644); crash_after();     return wfd < 0 ? -1 : 0; }
static int h_w_write(const void *b, uint32_t n) {
    ssize_t r; crash_before();
    if (g_fail_write_at && ++g_nwrite == g_fail_write_at) return -1;
    if (g_crash_at > 0 && g_op == g_crash_at && g_crash_mode == 1) { r = write(wfd, b, n / 2); (void)r; host_close_all(); longjmp(g_crash_jmp, 1); }
    if (g_corrupt_at && ++g_nwrite2 == g_corrupt_at && n > 100) { unsigned char *c = malloc(n); memcpy(c, b, n); c[100] ^= 0x55; r = write(wfd, c, n); free(c); return r == (ssize_t)n ? 0 : -1; }
    if (sdk_on() && g_w1_ok && ++g_wr_calls > 1) return 0;
    r = write(wfd, b, n); return r == (ssize_t)n ? 0 : -1;
}
static int h_w_close(void) { int r = 0; crash_before(); if (wfd >= 0) r = close(wfd); wfd = -1; crash_after(); return r; }
static int h_d_open(const char *p) { char q[1024]; P(q, p); if (dd) abort(); dd = opendir(q); return dd ? 0 : -1; }
static int h_d_next(char *name, int cap, int *isd) {
    struct dirent *e;
    while ((e = readdir(dd))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(name, (size_t)cap, "%s", e->d_name); *isd = e->d_type == DT_DIR; return 1;
    }
    return 0;
}
static void h_d_close(void) { if (dd) closedir(dd); dd = 0; }
static void *h_alloc(uint32_t n) { g_alloc_now += n; if (g_alloc_now > g_alloc_peak) g_alloc_peak = g_alloc_now; return malloc(n); }
static void h_free(void *p) { free(p); }
static uint32_t h_hf(void) { return g_heap_free; }
static uint32_t h_hl(void) { return g_heap_largest; }
static uint32_t h_now(void) { static uint32_t det; struct timespec t; if (getenv("NS_DET")) return det++; clock_gettime(CLOCK_MONOTONIC, &t); return (uint32_t)(t.tv_sec * 1000000u + t.tv_nsec / 1000); }
static int h_batt(void) { return g_battery; }
static uint32_t h_unix(void) { return g_unix; }
static void h_trace(const char *t, uint32_t a, uint32_t b) { if (getenv("NS_TRACE")) fprintf(stderr, "TRACE %s %u %u\n", t, a, b); }
const ns_os NS_HOST_OS = { h_exists, h_fsize, h_is_dir, h_mkdir, h_remove, h_read_all, h_r_open, h_r_read, h_r_close, h_w_open, h_w_write, h_w_close,
    h_d_open, h_d_next, h_d_close, h_alloc, h_free, h_hf, h_hl, h_now, h_batt, h_unix, h_trace
#ifdef NS_HAS_DIAG
    , h_io_diag
#endif
};
