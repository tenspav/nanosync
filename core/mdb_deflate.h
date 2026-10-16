/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_DEFLATE_H
#define MDB_DEFLATE_H
#include <stdint.h>
#include <stddef.h>
size_t mdb_zdeflate_bound(size_t n);
size_t mdb_zdeflate(const uint8_t *in, size_t n, uint8_t *out, size_t cap);
#define MDB_DEFLATE_WORK ((1u << 15) * 4u * 2u)
size_t mdb_zdeflate_w(const uint8_t *in, size_t n, uint8_t *out, size_t cap, void *work, size_t worksz);
#endif
