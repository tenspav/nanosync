/* PC platform for NanoSync: a directory tree that pretends to be the iPod + crash injection. */
#ifndef NS_HOST_H
#define NS_HOST_H
#include <setjmp.h>
#include "ns_engine.h"
extern const ns_os NS_HOST_OS;
extern char g_root[512];
extern long g_op, g_crash_at; extern int g_crash_mode;   /* mode 0: before the operation; 1: in the middle/after */
extern jmp_buf g_crash_jmp; extern int g_battery; extern uint32_t g_heap_free, g_heap_largest, g_unix; extern long g_alloc_now, g_alloc_peak;
extern int g_fail_write_at, g_corrupt_at;   /* g_corrupt_at: the N-th write comes out with one wrong byte (silent corruption) */
extern int g_unused_;   /* >0: the N-th write (w_write) fails (I/O error) */
void host_close_all(void);
#endif
