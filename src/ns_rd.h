/* NanoSync v0.0.0 - developed by tenspav */
#ifndef NS_RD_H
#define NS_RD_H
#include <stdint.h>
#include "ns_engine.h"
#ifndef NS_HAS_DIAG
typedef struct { int32_t rc; uint32_t req, got, calls; } ns_iodiag;
#endif
typedef int (*ns_fobj_read_fn)(void *o, uint32_t n, void *buf, uint32_t *got);
static inline int32_t ns_seq_read(ns_fobj_read_fn rd0, void *obj, void *buf, uint32_t n, ns_iodiag *dg) {
    ns_fobj_read_fn volatile rd = rd0; uint32_t have = 0, calls = 0;
    dg->rc = 0; dg->req = n; dg->got = 0; dg->calls = 0;
#ifdef NS_RD_LEGACY
    while (have < n && calls < 64) {
        uint64_t got64 = 0; int rc = rd(obj, n - have, (uint8_t *)buf + have, (uint32_t *)(void *)&got64); uint32_t got = (uint32_t)got64;
        calls++; dg->rc = rc; dg->calls = calls;
        if (got > n - have) return -1;
        have += got; dg->got = have;
        if (rc != 0) return have ? (int32_t)have : -1;
        if (got == 0) break;
    }
    return (int32_t)have;
#else
    while (have < n) {
        uint64_t got64 = 0; int rc = rd(obj, n - have, (uint8_t *)buf + have, (uint32_t *)(void *)&got64); uint32_t got = (uint32_t)got64;
        calls++; dg->rc = rc; dg->calls = calls;
        if (got > n - have) { dg->got = have; return -1; }
        have += got; dg->got = have;
        if (rc != 0 || got == 0) break;
    }
    return (int32_t)have;
#endif
}
#endif
