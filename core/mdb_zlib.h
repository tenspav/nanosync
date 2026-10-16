/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_ZLIB_H
#define MDB_ZLIB_H
#include <stdint.h>
#include <stddef.h>
int mdb_inflate(const uint8_t *in, size_t ilen, uint8_t *out, size_t ocap, size_t *olen, size_t *consumed);
size_t mdb_zstore_bound(size_t n);
size_t mdb_zstore(const uint8_t *in, size_t n, uint8_t *out);
uint32_t mdb_adler32(const uint8_t *p, size_t n);
#endif
