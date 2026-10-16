/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_TEXT_H
#define MDB_TEXT_H
#include <stdint.h>
#include <stddef.h>
uint32_t mdb_u8next(const char **pp);
size_t mdb_u8put(char *o, uint32_t c);
size_t mdb_u8_to_u16le(const char *s, uint8_t *out);
size_t mdb_u16le_to_u8(const uint8_t *in, size_t nbytes, char *out);
size_t mdb_norm_key(const char *s, char *out);
size_t mdb_casefold(const char *s, char *out);
uint32_t mdb_jump_letter(const char *key);
#endif
