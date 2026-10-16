/* NanoSync v0.0.0 - developed by tenspav */
#ifndef NS_ART_H
#define NS_ART_H
#include <stdint.h>
#include <stddef.h>
#include "tjpgd.h"
#define NS_FMT_N 4
typedef struct { uint16_t id, w, h, stride; uint32_t psize; } ns_fmt;
extern const ns_fmt NS_FMT[NS_FMT_N];
int ns_id3_cover(const uint8_t *p, uint32_t n, uint32_t *off, uint32_t *len);
#define NS_CV_MAXW 2048u
#define NS_PG_NCO 16u
typedef struct {
    const uint8_t *d; uint32_t n, pos, W, H, hmax, vmax, mcux, mcuy, ri, rleft, eobrun, idx, total, bitbuf, bitcnt, hit;
    uint8_t nc, sof, phase, ns, Ss, Se, Ah, Al, skip, ci[3]; int32_t pred[3];
    struct { uint8_t id, h, v, tq, td, ta, nac; uint32_t bw, bh, cbw, cbh; int16_t *dc; int8_t *ac; } c[3];
    uint16_t qt[4][NS_PG_NCO];
    struct { uint8_t ok; uint8_t vals[256]; int32_t maxcode[18], valptr[18], mincode[18]; } ht[2][4];
    uint32_t mx_i, my_i, auxsz[2]; uint8_t *aux[2];
} ns_pg;
typedef struct {
    JDEC jd; const uint8_t *src; uint32_t srclen, srcpos;
    uint8_t sc; uint32_t W0, H0, Ws, Hs, mx, my, x, y, bandrows;
    struct { uint32_t Wd, Hd, ox, oy, RB, flushed; uint16_t *mx0, *my0; uint8_t *wx0, *wy0; uint32_t *cw, *rw, *acc; uint8_t *out; } t[NS_FMT_N];
    int err, prog; ns_pg pg;
} ns_cv;
int  ns_cv_begin(ns_cv *c, const uint8_t *img, uint32_t len, uint8_t *pool, uint32_t poolsz, uint8_t *scratch, uint32_t scsz, uint8_t *const out[NS_FMT_N], uint8_t *aux, uint32_t auxsz, uint8_t *aux2, uint32_t aux2sz);
int  ns_cv_step(ns_cv *c, uint32_t nmcu);
uint32_t ns_cv_scratch_need(void);
int ns_adb_info(const uint8_t *old, uint32_t old_len, uint32_t *next_id, uint32_t *max_img_id);
uint32_t ns_adb_build(const uint8_t *old, uint32_t old_len, uint32_t n, const uint32_t *ids, const uint64_t *owner, const uint32_t *srcsz,
                      const uint32_t offs[NS_FMT_N][8], uint32_t next_id, uint8_t *out, uint32_t cap);
#endif
