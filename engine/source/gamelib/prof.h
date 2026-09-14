/*
 * prof.h -- lightweight startup / load profiler.
 *
 * Diagnostic scaffolding, not part of the engine proper.  Enabled by the
 * CMake option BOR_PROF (default ON), which defines BOR_PROF; with the
 * option off every macro below compiles to nothing.
 *
 * Log lines are buffered in RAM and written out by prof_flush(), so the
 * logging itself does not perturb what is being measured.  Output goes to
 * stdout (visible over nxlink) and is appended to OpenBOR_prof.log in the
 * working directory, i.e. next to the NRO.
 */
#ifndef BOR_PROF_H
#define BOR_PROF_H

#include <stdint.h>

/* A frame slower than this is logged as a hitch (in-game only). */
#define PROF_HITCH_MS 50.0

/* Where a frame's time went.  update() zeroes these every frame and reports
   them when the frame turns out to have been a hitch. */
enum
{
    PROF_PH_INPUT = 0,
    PROF_PH_SCRIPTS,
    PROF_PH_ENTS,
    PROF_PH_BG,
    PROF_PH_HUD,
    PROF_PH_SPRITEQ,
    PROF_PH_VWAIT,
    PROF_PH_PRESENT,
    PROF_PH_MUSIC,
    PROF_PH_COUNT
};

typedef struct prof_acc
{
    const char *name;
    uint64_t    us;     // total time spent inside
    uint64_t    calls;  // number of calls
    uint64_t    bytes;  // bytes moved, where that means anything
} prof_acc;

#ifdef BOR_PROF

uint64_t prof_us(void);
void     prof_log(const char *fmt, ...);
void     prof_flush(const char *reason);
void     prof_acc_add(prof_acc *a, uint64_t t0, uint64_t bytes);
void     prof_acc_report(prof_acc *a, int reset);

extern double      prof_phase[PROF_PH_COUNT];
extern const char *prof_phase_name[PROF_PH_COUNT];
void prof_phase_reset(void);
void prof_phase_report(void);

/* Ordered checkpoints within one frame.  Each mark names the region that
   follows it, so the report can say which stretch of code ran long even
   when that stretch contains no call worth wrapping on its own. */
void prof_mark(const char *label);
void prof_mark_reset(void);
void prof_mark_report(double threshold_ms);
#define PROF_MARK(l)    prof_mark(l)

#define PROF_T0(v)      uint64_t v = prof_us()
#define PROF_SINCE(v)   ((double)(prof_us() - (v)) / 1000.0)
/* variadic so the timed call may contain commas */
#define PROF_PHASE(idx, ...) \
    do { uint64_t _pht = prof_us(); __VA_ARGS__; prof_phase[idx] += PROF_SINCE(_pht); } while(0)

#else

#define prof_us()                   ((uint64_t)0)
#define prof_log(...)               ((void)0)
#define prof_flush(reason)          ((void)0)
#define prof_acc_add(a, t0, bytes)  ((void)0)
#define prof_acc_report(a, reset)   ((void)0)

#define prof_phase_reset()          ((void)0)
#define prof_phase_report()          ((void)0)
#define prof_mark(l)                ((void)0)
#define prof_mark_reset()           ((void)0)
#define prof_mark_report(t)         ((void)0)
#define PROF_MARK(l)                ((void)0)

#define PROF_T0(v)      uint64_t v = 0; (void)v
#define PROF_SINCE(v)   ((double)0.0)
#define PROF_PHASE(idx, ...)        do { __VA_ARGS__; } while(0)

#endif

#define PROF_ACC(sym)   static prof_acc sym __attribute__((unused)) = { #sym, 0, 0, 0 }

#endif
