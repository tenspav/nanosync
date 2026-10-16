/* NanoSync v0.0.0 - developed by tenspav */
#include "mdb_sig.h"
#include "sha1.h"
#include "calcHashAB.h"
static const uint8_t RND[23] = "ABCDEFGHIJKLMNOPQRSTUVW";
void mdb_hashab(uint8_t out[57], const uint8_t sha1[20], const uint8_t guid[8]) {
    calcHashAB(out, sha1, guid, RND);
}
static void zrange(mdb_sha1_t *c, const uint8_t *buf, size_t *pos, size_t to, size_t len) {
    (void)len; mdb_sha1_update(c, buf + *pos, to - *pos); *pos = to;
}
static void zeros(mdb_sha1_t *c, size_t *pos, size_t n) {
    static const uint8_t z[64] = {0};
    *pos += n;
    while (n) { size_t k = n > 64 ? 64 : n; mdb_sha1_update(c, z, k); n -= k; }
}
static int norm_sha1(const uint8_t *b, size_t len, uint8_t out[20]) {
    uint32_t hs; mdb_sha1_t c; size_t p = 0;
    static const uint8_t s4[2] = {4, 0};
    if (len < 12 || b[0]!='m'||b[1]!='h'||b[2]!='b'||b[3]!='d') return -1;
    hs = b[4] | (uint32_t)b[5]<<8 | (uint32_t)b[6]<<16 | (uint32_t)b[7]<<24;
    if (hs < 0xAB + 57 || hs > len) return -1;
    mdb_sha1_init(&c);
    zrange(&c, b, &p, 0x18, len); zeros(&c, &p, 8);
    zrange(&c, b, &p, 0x30, len); mdb_sha1_update(&c, s4, 2); p += 2;
    zrange(&c, b, &p, 0x32, len); zeros(&c, &p, 20);
    zrange(&c, b, &p, 0x58, len); zeros(&c, &p, 20);
    zrange(&c, b, &p, 0x72, len); zeros(&c, &p, 46);
    zrange(&c, b, &p, 0xAB, len); zeros(&c, &p, 57);
    zrange(&c, b, &p, len, len);
    mdb_sha1_final(&c, out);
    return 0;
}
int mdb_cdb_sign(uint8_t *b, size_t len, const uint8_t guid[8]) {
    uint8_t d[20]; if (norm_sha1(b, len, d)) return -1;
    mdb_hashab(b + 0xAB, d, guid);
    b[0x30] = 3; b[0x31] = 0;
    return 0;
}
int mdb_cdb_verify(const uint8_t *b, size_t len, const uint8_t guid[8]) {
    uint8_t d[20], h[57]; int i, bad = 0;
    if (norm_sha1(b, len, d)) return 0;
    mdb_hashab(h, d, guid);
    for (i = 0; i < 57; i++) bad |= h[i] ^ b[0xAB+i];
    return !bad && b[0x30] == 3 && b[0x31] == 0;
}
size_t mdb_cbk_size(size_t n) { return 57 + 20 + 20 * ((n + 1023) / 1024); }
void mdb_cbk_build(uint8_t *out, const uint8_t *loc, size_t n, const uint8_t guid[8]) {
    uint8_t *blocks = out + 77; size_t off, nb = 0; uint8_t fin[20];
    for (off = 0; off < n; off += 1024, nb++) {
        size_t k = n - off < 1024 ? n - off : 1024;
        mdb_sha1(loc + off, k, blocks + 20 * nb);
    }
    mdb_sha1(blocks, 20 * nb, fin);
    mdb_hashab(out, fin, guid);
    for (nb = 0; nb < 20; nb++) out[57 + nb] = fin[nb];
}
