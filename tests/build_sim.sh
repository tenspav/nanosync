#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(dirname "$HERE")
OUT=${1:-$REPO/build}
GUID=${NS_SIM_GUID:-0123456789ABCDEF}
C=$REPO/core
S=$REPO/src
mkdir -p "$OUT"
CORE_SRCS="$C/mdb_sig.c $C/sha1.c $C/calcHashAB.c $C/cipher.c $C/cipher_tables.c $C/edonr.c $C/hash.c $C/reduce.c $C/expand.c \
$C/mdb_zlib.c $C/mdb_text.c $C/mdb_cdb.c $C/mdb_sqlite.c $C/mdb_sqlgen.c $C/mdb_tags.c $C/mdb_deflate.c"
FLAGS="-O1 -g -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -DMDB_NO_MALLOC -DMDB_TAGS_DFLT_X=16 -I$S -I$C -I$HERE"
gcc $FLAGS -DNS_GUID_HEX="\"$GUID\"" -o "$OUT/ns_sim" "$HERE/sim_main.c" "$HERE/ns_host.c" "$S/ns_engine.c" "$S/ns_art.c" "$C/tjpgd.c" $CORE_SRCS
echo "built $OUT/ns_sim (guid $GUID)"
gcc -O1 -Wall -Wextra -DMDB_NO_MALLOC -I"$C" -o "$OUT/nsfix" "$HERE/nsfix.c" \
    "$C/mdb_cdb.c" "$C/mdb_zlib.c" "$C/mdb_text.c" "$C/mdb_sig.c" "$C/sha1.c" "$C/calcHashAB.c" "$C/cipher.c" "$C/cipher_tables.c" \
    "$C/edonr.c" "$C/hash.c" "$C/reduce.c" "$C/expand.c" "$C/mdb_sqlgen.c" "$C/mdb_sqlite.c"
echo "built $OUT/nsfix"
