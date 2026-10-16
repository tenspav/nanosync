/* NanoSync v0.0.0 - developed by tenspav */
#ifndef MDB_CDB_H
#define MDB_CDB_H
#include <stdint.h>
#include <stddef.h>
typedef struct {
    const char *title, *artist, *album, *genre, *composer, *album_artist;
    const char *location;
    uint32_t year, track_no, disc_no;
    uint32_t length_ms, size, bitrate_kbps, sample_rate;
    uint8_t vbr;
    const char *comment, *grouping, *subtitle, *description, *copyright, *content_provider, *lyrics;
    uint32_t total_tracks, total_discs, bpm, compilation, gapless_album, explicit_flag, rating, lyrics_flag;
    int64_t release_date;
    uint32_t artwork_id, artwork_count, artwork_size;
    uint32_t date_added, last_modified;
} mdb_track_t;
typedef struct {
    uint32_t track_id;
    uint64_t db_track_id;
    uint32_t album_id, artist_id, composer_id;
    uint64_t album_sql_id, artist_sql_id;
    uint64_t mhip_pid[2];
    uint32_t position;
} mdb_assigned_t;
enum {
    MDB_OK = 0, MDB_E_FORMAT = -1, MDB_E_SPACE = -2, MDB_E_SCRATCH = -3, MDB_E_UNSUPPORTED = -4, MDB_E_ARG = -5
};
int mdb_cdb_unpack(const uint8_t *phys, size_t plen, uint8_t *logical, size_t cap, size_t *llen);
size_t mdb_cdb_pack_bound(size_t llen);
int mdb_cdb_pack(const uint8_t *logical, size_t llen, uint8_t *phys, size_t cap, size_t *plen, const uint8_t guid[8]);
size_t mdb_cdb_out_bound(size_t in_len, uint32_t n);
size_t mdb_cdb_scratch_need(size_t in_len, uint32_t n);
int mdb_cdb_add_tracks(const uint8_t *in, size_t in_len, const mdb_track_t *tr, uint32_t n,
                       uint8_t *out, size_t out_cap, size_t *out_len, mdb_assigned_t *asg,
                       void *scratch, size_t scratch_size);
#endif
