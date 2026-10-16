/* NanoSync v0.0.0 - developed by tenspav */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ns_host.h"
static const char *PH[] = {"INIT","RECOVER","SCAN","TAGS","LIB","READY","CHECK","PLAN","BAK","JRN","COPY","CPVER","TAIL","ART","GEN","WRITE","VERIFY","RMSRC","JDEL","DONE","FAIL"};
int main(int argc, char **argv) {
    uint32_t block_kb = 1900; int i; volatile int autoimp = 0, stats = 0, noimp = 0; ns_t *s; void *blk; volatile long steps = 0; volatile int imported = 0;
    if (argc < 2) { fprintf(stderr, "usage: ns_sim <root> ...\n"); return 2; }
    snprintf(g_root, sizeof g_root, "%s", argv[1]);
    for (i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--block")) block_kb = (uint32_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--battery")) g_battery = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--crash")) g_crash_at = atol(argv[++i]);
        else if (!strcmp(argv[i], "--mode")) g_crash_mode = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--failwrite")) g_fail_write_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--corrupt")) g_corrupt_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--auto-import")) autoimp = 1;
        else if (!strcmp(argv[i], "--no-import")) noimp = 1;
        else if (!strcmp(argv[i], "--stats")) stats = 1;
        else if (!strcmp(argv[i], "--unix")) g_unix = (uint32_t)strtoul(argv[++i], 0, 10);
    }
    blk = malloc(block_kb * 1024u);
    s = ns_create(&NS_HOST_OS, blk, block_kb * 1024u);
    if (!s) { printf("RESULT create_failed (block %u KB < minimum %u KB)\n", block_kb, ns_min_block() / 1024); return 3; }
    if (setjmp(g_crash_jmp)) { printf("CRASHED at op %ld (mode %d)\n", g_op, g_crash_mode); return 42; }
    for (;;) {
        ns_phase ph = ns_get_phase(s);
        if (ph == NS_P_READY && autoimp && !imported && !noimp && ns_nfiles(s)) { ns_request_import(s); imported = 1; }
        else if (ph == NS_P_READY || ph == NS_P_DONE || ph == NS_P_FAIL) break;
        ns_step(s, 12000);
        if (++steps > 5000000) { printf("RESULT timeout\n"); return 4; }
    }
    printf("PHASE %s\nRESULT %d\nMSG %s\nMSG2 %s\nFILES %u\n", PH[ns_get_phase(s)], (int)ns_get_result(s), ns_get_msg(s), ns_get_msg2(s), ns_nfiles(s));
    for (i = 0; i < (int)ns_nfiles(s); i++) { const ns_file *f = ns_file_at(s, i); char a[80], b[80]; ns_label(s, (uint32_t)i, a, sizeof a, b, sizeof b); printf("FILE %d %s size=%u status=%d [%s] title=%s artist=%s dest=%s\n", i, f->name, f->size, f->status, f->reason, a, b, f->dest); }
#ifdef NS_HAS_DIAG
    if (ns_get_diag(s)[0]) printf("DIAG\n%s\n", ns_get_diag(s));
#endif
    printf("LIB before=%d after=%d dry=%d\nOPS %ld\n", ns_lib_before(s), ns_lib_after(s), ns_dryrun(s), g_op);
    if (stats) {
        const ns_stats *st = ns_get_stats(s);
        printf("MEM block=%u work=%u need_est=%u peak=%u\n", ns_block_bytes(s), ns_mem_have(s), ns_mem_need(s), ns_mem_peak(s));
        printf("TIME_US tags=%u cdb=%u sql=%u deflate=%u sign=%u gen_total=%u max_unit=%u\n", st->t_tags_us, st->t_cdb_us, st->t_sql_us, st->t_defl_us, st->t_sign_us, st->t_gen_us, st->max_unit_us);
        printf("SIZES cdb_phys_in=%u cdb_logical_in=%u cdb_logical_out=%u cdb_phys_out=%u lib=%u loc=%u dyn=%u cbk=%u scratch_sql=%u\n", st->cdb_phys_in, st->cdb_logical_in, st->cdb_logical_out, st->cdb_phys_out, st->sql_len[0], st->sql_len[1], st->sql_len[2], st->cbk_len, st->scratch_sql_used);
    }
    return 0;
}
