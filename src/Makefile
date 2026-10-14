# NanoSync app Makefile, for the NanoApps SDK build system.
# Usage: copy src/ to <NanoApps>/apps/nanosync and core/ to <NanoApps>/apps/nanosync/core, then
#        make NS_GUID=<16 hex digits of the iPod serial>
# (tools/build.sh does exactly that.)
APP_NAME    := nanosync
ifndef NS_GUID
$(error NS_GUID is not set: pass the iPod's 16 hex digit serial number, e.g. make NS_GUID=0123456789ABCDEF)
endif
# portable C core (HashAB, CDB writer, SQLite writer, tags, deflate, TJpgDec)
CORE        ?= core
SRCS        := nanosync.c ns_engine.c ns_art.c $(CORE)/tjpgd.c ns_hb.c \
               $(CORE)/mdb_sig.c $(CORE)/sha1.c $(CORE)/calcHashAB.c $(CORE)/cipher.c $(CORE)/cipher_tables.c $(CORE)/edonr.c \
               $(CORE)/hash.c $(CORE)/reduce.c $(CORE)/expand.c $(CORE)/mdb_zlib.c $(CORE)/mdb_text.c $(CORE)/mdb_cdb.c \
               $(CORE)/mdb_sqlite.c $(CORE)/mdb_sqlgen.c $(CORE)/mdb_tags.c $(CORE)/mdb_deflate.c
LV_SURFACE  := 1
EXTRA_CFLAGS += -O2 -mword-relocations -I$(CORE) -DMDB_NO_MALLOC -DMDB_TAGS_DFLT_X=16 -DNS_GUID_HEX='"$(NS_GUID)"'
include ../../sdk/hb_app.mk
