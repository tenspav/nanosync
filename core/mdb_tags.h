/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_TAGS_H
#define MDB_TAGS_H
#include <stdint.h>
#include <stddef.h>
#define MDB_TAG_STR 256
typedef struct {
    char title[MDB_TAG_STR], artist[MDB_TAG_STR], album[MDB_TAG_STR], genre[MDB_TAG_STR], composer[MDB_TAG_STR], album_artist[MDB_TAG_STR];
    uint32_t year, track, disc;
    uint32_t sample_rate, bitrate_kbps, duration_ms, channels, layer, mpeg;
    uint8_t vbr;
    uint32_t total_tracks, total_discs, bpm, compilation, gapless_album, explicit_flag  , rating  ;
    int64_t release_date;
    const char *comment, *grouping, *subtitle, *description, *copyright, *content_provider, *lyrics;
    uint32_t lyrics_frames, lyrics_flag;
    char ignored[256];
    char *xbuf; uint32_t xcap;
} mdb_tags_t;
int mdb_tags_read(const uint8_t *b, size_t len, mdb_tags_t *t);
int mdb_tags_read_ex(const uint8_t *b, size_t len, size_t total, mdb_tags_t *t);
#endif
