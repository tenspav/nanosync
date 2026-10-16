/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_SQLGEN_H
#define MDB_SQLGEN_H
#include <stdint.h>
#include <stddef.h>
#include "mdb_sqlite.h"
enum { MDB_SQL_E_FORMAT = -1, MDB_SQL_E_SPACE = -2, MDB_SQL_E_MEM = -3, MDB_SQL_E_UNSUPPORTED = -4 };
typedef struct { uint8_t *buf[3]; size_t cap[3], len[3]; size_t scratch_used; } mdb_sqlgen_out_t;
int mdb_sqlgen_build(const uint8_t *cdb, size_t len, mdb_sqlgen_out_t *out, void *scratch, size_t scratch_size);
#endif
