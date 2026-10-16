/* NanoSync v0.0.0 - developed by tenspav */
#include "hb_sdk.h"
#include "hb_heap.h"
#include "lvgl/lvgl.h"
#include "ns_engine.h"
#include "ns_hb.h"
#define RESERVE_BYTES   (1536u * 1024u)
#define BLOCK_MAX       (2048u * 1024u)
#define IDLE_FREE_MS    (5u * 60u * 1000u)
static ns_t *g_ns;
static uint8_t *g_blk;
static int g_freed, g_nomem;
static uint32_t g_heap0, g_large0;
static lv_obj_t *lbl_title, *lbl_state, *lbl_state2, *lbl_lib, *lbl_list, *lbl_stage, *bar, *btn_imp, *lbl_mem;
static char listbuf[1400];
static void ui_set_enabled(lv_obj_t *b, int en) { if (en) lv_obj_remove_state(b, LV_STATE_DISABLED); else lv_obj_add_state(b, LV_STATE_DISABLED); }
static void block_free(void) {
    ns_hb_close_all();
    if (g_blk) { if (HB_APP_AUX_BLOCK == g_blk) HB_APP_AUX_BLOCK = 0; hb_os_free(g_blk); }
    g_blk = 0; g_ns = 0;
}
static int block_alloc(void) {
    uint32_t f = hb_os_heap_free(), lg = hb_os_heap_largest(), cap, tries;
    g_heap0 = f; g_large0 = lg;
    if (HB_APP_AUX_BLOCK) { hb_os_free(HB_APP_AUX_BLOCK); HB_APP_AUX_BLOCK = 0; }
    cap = f > RESERVE_BYTES ? f - RESERVE_BYTES : 0;
    if (cap > lg) cap = lg;
    if (cap > BLOCK_MAX) cap = BLOCK_MAX;
    for (tries = 0; tries < 4 && cap >= ns_min_block(); tries++) {
        uint8_t *b = (uint8_t *)hb_os_alloc(cap);
        if (b) {
            if (hb_os_heap_free() >= RESERVE_BYTES - 256u * 1024u) {
                g_blk = b; HB_APP_AUX_BLOCK = b;
                g_ns = ns_create(&NS_HB_OS, b, cap);
                if (g_ns) { hb_trace_log("NSBLK   ", cap, hb_os_heap_free()); return 0; }
            }
            hb_os_free(b); HB_APP_AUX_BLOCK = 0; g_blk = 0;
        }
        cap = cap > 320u * 1024u ? cap - 256u * 1024u : 0;
    }
    return -1;
}
static void on_import(lv_event_t *e) { (void)e; if (g_ns) ns_request_import(g_ns); }
static void on_rescan(lv_event_t *e) {
    (void)e;
    if (!g_ns) { g_freed = 0; g_nomem = 0; if (block_alloc()) g_nomem = 1; return; }
    ns_request_rescan(g_ns);
}
static void on_free(lv_event_t *e) {
    ns_phase p; (void)e;
    if (!g_ns) return;
    p = ns_get_phase(g_ns);
    if (!(p == NS_P_READY || p == NS_P_DONE || p == NS_P_FAIL)) return;
    block_free(); g_freed = 1;
    hb_trace_log("NSFREE  ", g_heap0, hb_os_heap_free());
}
static void frame_cb(void) {
    if (g_ns) {
        ns_step(g_ns, 12000);
        { ns_phase p = ns_get_phase(g_ns);
          if (lv_display_get_inactive_time(0) > IDLE_FREE_MS && (p == NS_P_READY || p == NS_P_DONE || p == NS_P_FAIL)) { block_free(); g_freed = 1; } }
    }
}
static const char *stage_name(ns_stage s) {
    switch (s) { case NS_ST_COPY: return "Copying"; case NS_ST_GEN: return "Generating"; case NS_ST_WRITE: return "Writing"; case NS_ST_VERIFY: return "Verifying"; default: return ""; }
}
static void refresh(lv_timer_t *t) {
    (void)t;
    if (!g_ns) {
        lv_label_set_text(lbl_state, g_nomem ? "Not enough memory" : g_freed ? "Memory released - you can exit" : "No memory");
        lv_label_set_text(lbl_state2, g_nomem ? "Close other apps / restart the iPod and tap Reload." : "Tap Reload to continue.");
        lv_label_set_text(lbl_lib, ""); lv_label_set_text(lbl_list, ""); lv_label_set_text(lbl_stage, ""); lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_label_set_text_fmt(lbl_mem, "free heap %u KB, largest block %u KB", (unsigned)(hb_os_heap_free() / 1024u), (unsigned)(hb_os_heap_largest() / 1024u));
        ui_set_enabled(btn_imp, 0);
        return;
    }
    {
        ns_phase p = ns_get_phase(g_ns); uint32_t i, n = ns_nfiles(g_ns), o = 0; ns_stage st = ns_get_stage(g_ns);
        lv_label_set_text(lbl_state, ns_get_msg(g_ns));
        lv_label_set_text(lbl_state2, ns_get_msg2(g_ns));
        if (ns_lib_before(g_ns) >= 0) {
            if (ns_lib_after(g_ns) >= 0) lv_label_set_text_fmt(lbl_lib, "Library: %d -> %d tracks%s", ns_lib_before(g_ns), ns_lib_after(g_ns), ns_dryrun(g_ns) ? " (dry run)" : "");
            else lv_label_set_text_fmt(lbl_lib, "Library: %d tracks%s", ns_lib_before(g_ns), ns_dryrun(g_ns) ? "  [DRY RUN]" : "");
        } else lv_label_set_text(lbl_lib, "Library: ?");
        listbuf[0] = 0;
        if (p == NS_P_FAIL || p == NS_P_DONE) {
            const char *dg = ns_get_diag(g_ns);
            while (*dg && o < 420) listbuf[o++] = *dg++;
            if (o) listbuf[o++] = (char)10;
            listbuf[o] = 0;
        }
        for (i = 0; i < n && o + 72 < sizeof listbuf; i++) {
            char a[28], b[18]; const ns_file *f = ns_file_at(g_ns, i); uint32_t k = 0;
            ns_label(g_ns, i, a, sizeof a, b, sizeof b);
            listbuf[o++] = (char)('0' + (i + 1) / 10 % 10); listbuf[o++] = (char)('0' + (i + 1) % 10); listbuf[o++] = '.'; listbuf[o++] = ' ';
            if (f->status < 0) { listbuf[o++] = 'X'; listbuf[o++] = ' '; }
            while (a[k] && o < sizeof listbuf - 40) listbuf[o++] = a[k++];
            if (b[0]) { listbuf[o++] = ' '; listbuf[o++] = '-'; listbuf[o++] = ' '; k = 0; while (b[k] && o < sizeof listbuf - 24) listbuf[o++] = b[k++]; }
            listbuf[o++] = '\n'; listbuf[o] = 0;
        }
        lv_label_set_text(lbl_list, listbuf);
        lv_label_set_text_fmt(lbl_stage, "%s %u%%", stage_name(st), (unsigned)ns_get_progress(g_ns));
        lv_bar_set_value(bar, (int)ns_get_progress(g_ns), LV_ANIM_OFF);
        lv_label_set_text_fmt(lbl_mem, "RAM block %uK peak %uK  bat %d%%", (unsigned)(ns_block_bytes(g_ns) / 1024u), (unsigned)(ns_mem_peak(g_ns) / 1024u), NS_HB_OS.battery_pct());
        ui_set_enabled(btn_imp, p == NS_P_READY && n > 0);
    }
}
static lv_obj_t *mkbtn(lv_obj_t *scr, const char *txt, int x, int w, lv_event_cb_t cb) {
    lv_obj_t *b = lv_button_create(scr), *l;
    lv_obj_set_size(b, w, 40); lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, x, -8);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, 0);
    l = lv_label_create(b); lv_label_set_text(l, txt); lv_obj_center(l);
    return b;
}
HB_APP_ENTRY(payload_entry) {
    lv_obj_t *scr = lv_screen_active(), *box;
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101826), 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(0xffffff), 0);
    lbl_title = lv_label_create(scr); lv_label_set_text(lbl_title, "NanoSync"); lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0); lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 6);
    lbl_state = lv_label_create(scr); lv_obj_set_width(lbl_state, 224); lv_label_set_long_mode(lbl_state, LV_LABEL_LONG_WRAP); lv_obj_set_style_text_color(lbl_state, lv_color_hex(0x7fd3ff), 0); lv_obj_align(lbl_state, LV_ALIGN_TOP_MID, 0, 38);
    lbl_state2 = lv_label_create(scr); lv_obj_set_width(lbl_state2, 224); lv_label_set_long_mode(lbl_state2, LV_LABEL_LONG_WRAP); lv_obj_set_style_text_color(lbl_state2, lv_color_hex(0xffd479), 0); lv_obj_align(lbl_state2, LV_ALIGN_TOP_MID, 0, 86);
    lbl_lib = lv_label_create(scr); lv_obj_set_width(lbl_lib, 224); lv_obj_align(lbl_lib, LV_ALIGN_TOP_MID, 0, 138);
    box = lv_obj_create(scr); lv_obj_set_size(box, 228, 150); lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x1c2740), 0); lv_obj_set_style_pad_all(box, 4, 0); lv_obj_set_style_border_width(box, 0, 0);
    lbl_list = lv_label_create(box); lv_obj_set_width(lbl_list, 216); lv_label_set_long_mode(lbl_list, LV_LABEL_LONG_CLIP); lv_label_set_text(lbl_list, "");
    lbl_stage = lv_label_create(scr); lv_obj_align(lbl_stage, LV_ALIGN_TOP_LEFT, 8, 318);
    bar = lv_bar_create(scr); lv_obj_set_size(bar, 224, 12); lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 340);
    lbl_mem = lv_label_create(scr); lv_obj_set_width(lbl_mem, 228); lv_label_set_long_mode(lbl_mem, LV_LABEL_LONG_CLIP); lv_obj_align(lbl_mem, LV_ALIGN_TOP_LEFT, 6, 358);
    btn_imp = mkbtn(scr, "Import", 6, 70, on_import);
    mkbtn(scr, "Reload", 80, 78, on_rescan);
    mkbtn(scr, "Free RAM", 162, 74, on_free);
    if (block_alloc()) g_nomem = 1;
    hb_lv_set_frame_cb(frame_cb);
    lv_timer_create(refresh, 200, 0); refresh(0);
}
