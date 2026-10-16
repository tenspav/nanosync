/* NanoSync v0.0.0 - developed by tenspav */
#ifndef NS_ENGINE_H
#define NS_ENGINE_H
#include <stdint.h>
#include <stddef.h>
#include "mdb_tags.h"
#define NS_MAX_FILES   20u
#define NS_NAME        128
#define NS_PIECE       32768u
#define NS_MSG         160
#define NS_MIN_BATT    40
typedef struct { int32_t rc; uint32_t req, got, calls; } ns_iodiag;
#define NS_HAS_DIAG 1
typedef struct ns_os {
    int      (*exists)(const char *path);
    int32_t  (*fsize)(const char *path);
    int      (*is_dir)(const char *path);
    int      (*mkdir)(const char *path);
    int      (*remove)(const char *path);
    uint32_t (*read_all)(const char *path, void *buf, uint32_t max);
    int      (*r_open)(const char *path);
    int32_t  (*r_read)(void *buf, uint32_t n);
    void     (*r_close)(void);
    int      (*w_open)(const char *path);
    int      (*w_write)(const void *buf, uint32_t n);
    int      (*w_close)(void);
    int      (*d_open)(const char *path);
    int      (*d_next)(char *name, int cap, int *is_dir);
    void     (*d_close)(void);
    void    *(*alloc)(uint32_t n);
    void     (*mfree)(void *p);
    uint32_t (*heap_free)(void);
    uint32_t (*heap_largest)(void);
    uint32_t (*now_us)(void);
    int      (*battery_pct)(void);
    uint32_t (*unix_time)(void);
    void     (*trace)(const char *tag, uint32_t a, uint32_t b);
    void     (*io_diag)(ns_iodiag *out);
} ns_os;
typedef enum {
    NS_P_INIT = 0, NS_P_RECOVER, NS_P_SCAN, NS_P_TAGS, NS_P_LIB, NS_P_READY,
    NS_P_CHECK, NS_P_PLAN, NS_P_BAK, NS_P_JRN, NS_P_COPY, NS_P_CPVER, NS_P_TAIL, NS_P_ART, NS_P_GEN, NS_P_WRITE, NS_P_VERIFY, NS_P_RMSRC, NS_P_JDEL,
    NS_P_DONE, NS_P_FAIL
} ns_phase;
typedef enum { NS_ST_NONE = 0, NS_ST_COPY, NS_ST_GEN, NS_ST_WRITE, NS_ST_VERIFY } ns_stage;
typedef enum {
    NS_R_NONE = 0, NS_R_OK, NS_R_DRYRUN_OK, NS_R_RESTORED, NS_R_REFUSED, NS_R_ROLLED_BACK, NS_R_CRITICAL
} ns_result;
typedef struct {
    char     name[NS_NAME];
    uint32_t size;
    int8_t   status;
    char     reason[40];
    uint8_t  have_tags;
    int8_t   cv;
    uint8_t  cs;
    char     dest[40];
    uint8_t  sha[20];
} ns_file;
typedef struct ns ns_t;
uint32_t ns_state_bytes(void);
uint32_t ns_min_block(void);
ns_t *ns_create(const ns_os *os, void *block, uint32_t bytes);
int  ns_step(ns_t *s, uint32_t budget_us);
void ns_request_import(ns_t *s);
void ns_request_rescan(ns_t *s);
int  ns_can_cancel(const ns_t *s);
ns_phase    ns_get_phase(const ns_t *s);
ns_stage    ns_get_stage(const ns_t *s);
uint32_t    ns_get_progress(const ns_t *s);
const char *ns_get_msg(const ns_t *s);
const char *ns_get_diag(const ns_t *s);
const char *ns_get_msg2(const ns_t *s);
ns_result   ns_get_result(const ns_t *s);
uint32_t    ns_nfiles(const ns_t *s);
const ns_file *ns_file_at(const ns_t *s, uint32_t i);
const mdb_tags_t *ns_tags_at(const ns_t *s, uint32_t i);
void ns_label(const ns_t *s, uint32_t i, char *out, uint32_t cap, char *out2, uint32_t cap2);
int  ns_dryrun(const ns_t *s);
int  ns_lib_before(const ns_t *s);
int  ns_lib_after(const ns_t *s);
uint32_t ns_mem_need(const ns_t *s);
uint32_t ns_mem_have(const ns_t *s);
uint32_t ns_mem_peak(const ns_t *s);
uint32_t ns_block_bytes(const ns_t *s);
const char *ns_log_text(const ns_t *s, uint32_t *len);
typedef struct {
    uint32_t t_gen_us, t_tags_us, t_cdb_us, t_sql_us, t_defl_us, t_sign_us, t_cover_us;
    uint32_t max_unit_us;
    uint32_t cdb_phys_in, cdb_logical_in, cdb_logical_out, cdb_phys_out, sql_len[3], cbk_len;
    uint32_t scratch_cdb_used, scratch_sql_used;
} ns_stats;
const ns_stats *ns_get_stats(const ns_t *s);
#define NS_DIR_IMP   "/Apps/Data/Importar"
#define NS_F_JOURNAL "/Apps/Data/Importar/journal.txt"
#define NS_F_LOG     "/Apps/Data/Importar/log.txt"
#define NS_F_DRY     "/Apps/Data/Importar/dryrun.txt"
#define NS_F_NOART   "/Apps/Data/Importar/noart.txt"
#define NS_F_INIT    "/Apps/Data/Importar/ns_init.txt"
#define NS_DIR_ITUNES "/iPod_Control/iTunes"
#define NS_DIR_ART   "/iPod_Control/Artwork"
#define NS_DIR_ITL   "/iPod_Control/iTunes/iTunes Library.itlp"
#endif
