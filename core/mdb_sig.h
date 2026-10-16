/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_SIG_H
#define MDB_SIG_H
#include <stdint.h>
#include <stddef.h>
void mdb_hashab(uint8_t out[57], const uint8_t sha1[20], const uint8_t guid[8]);
int mdb_cdb_sign(uint8_t *buf, size_t len, const uint8_t guid[8]);
int mdb_cdb_verify(const uint8_t *buf, size_t len, const uint8_t guid[8]);
size_t mdb_cbk_size(size_t loc_len);
void mdb_cbk_build(uint8_t *out, const uint8_t *loc, size_t loc_len, const uint8_t guid[8]);
#endif
