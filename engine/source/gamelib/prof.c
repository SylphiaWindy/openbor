/*
 * prof.c -- see prof.h.  Whole file compiles to nothing without BOR_PROF.
 */
#ifdef BOR_PROF

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "prof.h"

#define PROF_BUFSIZE (192 * 1024)
#define PROF_LOGFILE "OpenBOR_prof.log"

static char     prof_buf[PROF_BUFSIZE];
static size_t   prof_len = 0;
static int      prof_truncated = 0;
static uint64_t prof_base = 0;

uint64_t prof_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

void prof_log(const char *fmt, ...)
{
    va_list args;
    uint64_t t;
    int n;

    if(!prof_base)
    {
        prof_base = prof_us();
    }
    t = prof_us() - prof_base;

    // leave room for one more full line, otherwise stop recording
    if(prof_len + 1024 >= PROF_BUFSIZE)
    {
        prof_truncated = 1;
        return;
    }

    n = snprintf(prof_buf + prof_len, PROF_BUFSIZE - prof_len, "[prof] %10.3f  ", t / 1000.0);
    if(n > 0)
    {
        prof_len += n;
    }

    va_start(args, fmt);
    n = vsnprintf(prof_buf + prof_len, PROF_BUFSIZE - prof_len, fmt, args);
    va_end(args);
    if(n > 0)
    {
        prof_len += n;
        if(prof_len >= PROF_BUFSIZE - 1)
        {
            prof_len = PROF_BUFSIZE - 2;
            prof_truncated = 1;
        }
    }

    prof_buf[prof_len++] = '\n';
    prof_buf[prof_len] = 0;
}

void prof_flush(const char *reason)
{
    FILE *fp;

    if(!prof_len)
    {
        return;
    }

    printf("[prof] ======== %s ========\n", reason ? reason : "");
    fwrite(prof_buf, 1, prof_len, stdout);
    if(prof_truncated)
    {
        printf("[prof] BUFFER TRUNCATED -- flush more often\n");
    }
    fflush(stdout);

    fp = fopen(PROF_LOGFILE, "a");
    if(fp)
    {
        fprintf(fp, "======== %s ========\n", reason ? reason : "");
        fwrite(prof_buf, 1, prof_len, fp);
        if(prof_truncated)
        {
            fputs("[prof] BUFFER TRUNCATED -- flush more often\n", fp);
        }
        fclose(fp);
    }

    prof_len = 0;
    prof_truncated = 0;
}

double prof_phase[PROF_PH_COUNT];

const char *prof_phase_name[PROF_PH_COUNT] =
{
    "input", "scripts", "ents", "bg", "hud", "spriteq", "vwait", "present", "music"
};

void prof_phase_reset(void)
{
    memset(prof_phase, 0, sizeof(prof_phase));
}

// Only the phases worth naming; a frame that stalled has one big number and
// a dozen zeroes, and printing the zeroes buries it.
void prof_phase_report(void)
{
    char line[512];
    size_t n = 0;
    int i;

    line[0] = 0;
    for(i = 0; i < PROF_PH_COUNT; i++)
    {
        if(prof_phase[i] >= 1.0 && n + 48 < sizeof(line))
        {
            int w = snprintf(line + n, sizeof(line) - n, "%s%s %.1f",
                             n ? "  " : "", prof_phase_name[i], prof_phase[i]);
            if(w > 0)
            {
                n += w;
            }
        }
    }
    prof_log("    phases (ms): %s", n ? line : "(every phase under 1 ms -- the time went somewhere untimed)");
}

#define PROF_MARK_MAX 48

static const char *prof_mark_label[PROF_MARK_MAX];
static uint64_t    prof_mark_time[PROF_MARK_MAX];
static int         prof_mark_n = 0;

void prof_mark_reset(void)
{
    prof_mark_n = 0;
}

void prof_mark(const char *label)
{
    if(prof_mark_n < PROF_MARK_MAX)
    {
        prof_mark_label[prof_mark_n] = label;
        prof_mark_time[prof_mark_n] = prof_us();
        prof_mark_n++;
    }
}

void prof_mark_report(double threshold_ms)
{
    int i, printed = 0;

    for(i = 1; i < prof_mark_n; i++)
    {
        double d = (double)(prof_mark_time[i] - prof_mark_time[i - 1]) / 1000.0;
        if(d >= threshold_ms)
        {
            prof_log("      %-16s %9.3f ms", prof_mark_label[i - 1], d);
            printed = 1;
        }
    }
    // an early return leaves the tail unmarked; say how far the frame got
    if(prof_mark_n > 0)
    {
        prof_log("      last mark reached: %s (%d marks)",
                 prof_mark_label[prof_mark_n - 1], prof_mark_n);
    }
    if(!printed)
    {
        prof_log("      (no region over %.1f ms)", threshold_ms);
    }
}

void prof_acc_add(prof_acc *a, uint64_t t0, uint64_t bytes)
{
    a->us += prof_us() - t0;
    a->calls++;
    a->bytes += bytes;
}

void prof_acc_reset(prof_acc *a)
{
    a->us = 0;
    a->calls = 0;
    a->bytes = 0;
}

void prof_acc_report(prof_acc *a, int reset)
{
    prof_log("    %-26s %8llu calls  %10.3f ms  %llu bytes",
             a->name,
             (unsigned long long)a->calls,
             a->us / 1000.0,
             (unsigned long long)a->bytes);
    if(reset)
    {
        a->us = 0;
        a->calls = 0;
        a->bytes = 0;
    }
}

#else
typedef int prof_translation_unit_not_empty;
#endif

#ifdef BOR_PROF
/* Allocation counters; see safealloc.h. */
uint64_t bor_alloc_count = 0;
uint64_t bor_alloc_bytes = 0;
uint64_t bor_alloc_us    = 0;
#endif

#ifdef BOR_PROF
/*
 * Which lines allocate.
 *
 * The allocator is ~2 s of a level load on the Switch (697 ns a call there
 * against 21 ns on a desktop), so pooling is worth it -- but only for the
 * sites that dominate, and those are not obvious from the object counts.
 * safeMalloc already knows its caller; keep a table keyed by it.
 */
#define BOR_SITE_SLOTS 65536

typedef struct
{
    const char *file;
    int         line;
    uint64_t    calls;
    uint64_t    bytes;
    uint64_t    us;
} bor_site;

static bor_site bor_sites[BOR_SITE_SLOTS];
static uint64_t bor_site_dropped;

void bor_alloc_site(const char *file, int line, uint64_t bytes, uint64_t us)
{
    /* String literals, so the pointer identifies the file. */
    size_t h = (((size_t)file >> 4) ^ (size_t)(line * 2654435761u)) & (BOR_SITE_SLOTS - 1);
    size_t i;

    for(i = 0; i < 64; i++)         /* bounded probe; the table is sparse */
    {
        bor_site *s = &bor_sites[(h + i) & (BOR_SITE_SLOTS - 1)];
        if(!s->file || (s->file == file && s->line == line))
        {
            s->file = file;
            s->line = line;
            s->calls++;
            s->bytes += bytes;
            s->us += us;
            return;
        }
    }
    bor_site_dropped++;
}

/* Per line is too fine: scriptlib spreads its allocations over hundreds of
   them, so the worst lines are all model loading and the parser never shows.
   Summarise by file first, then list the worst individual lines. */
static void bor_alloc_file_report(void)
{
    struct { const char *file; uint64_t calls, us; } agg[64];
    int n = 0, i, j, r;
    char used[64];
    uint64_t table_total = 0;
    int distinct = 0;

    /* Cross-check: this must agree with the engine-wide counter, or the table
       is losing rows and the ranking below cannot be trusted. */
    for(i = 0; i < BOR_SITE_SLOTS; i++)
        if(bor_sites[i].file) { table_total += bor_sites[i].calls; distinct++; }
    prof_log("      [table holds %llu calls over %d sites]",
             (unsigned long long)table_total, distinct);

    memset(agg, 0, sizeof(agg));
    for(i = 0; i < BOR_SITE_SLOTS; i++)
    {
        if(!bor_sites[i].file) continue;
        for(j = 0; j < n; j++) if(agg[j].file == bor_sites[i].file) break;
        if(j == n)
        {
            if(n >= 64) continue;
            agg[n].file = bor_sites[i].file; n++;
        }
        agg[j].calls += bor_sites[i].calls;
        agg[j].us    += bor_sites[i].us;
    }

    memset(used, 0, sizeof(used));
    for(r = 0; r < 8; r++)
    {
        int best = -1;
        for(i = 0; i < n; i++)
            if(!used[i] && (best < 0 || agg[i].calls > agg[best].calls)) best = i;
        if(best < 0 || !agg[best].calls) break;
        used[best] = 1;
        {
            const char *f = strrchr(agg[best].file, '/');
            prof_log("      %-26s %9llu x %8.3f ms",
                     f ? f + 1 : agg[best].file,
                     (unsigned long long)agg[best].calls,
                     (double)agg[best].us / 1000.0);
        }
    }
}

void bor_alloc_site_report(int top)
{
    bor_alloc_file_report();
    int r;
    char seen[BOR_SITE_SLOTS];
    size_t i;

    memset(seen, 0, sizeof(seen));
    for(r = 0; r < top; r++)
    {
        size_t best = BOR_SITE_SLOTS;
        for(i = 0; i < BOR_SITE_SLOTS; i++)
            if(!seen[i] && bor_sites[i].file &&
               (best == BOR_SITE_SLOTS || bor_sites[i].calls > bor_sites[best].calls))
                best = i;
        if(best == BOR_SITE_SLOTS) break;
        seen[best] = 1;
        {
            const char *f = strrchr(bor_sites[best].file, '/');
            prof_log("      %-22s:%-5d %9llu x %8.3f ms %8llu KB",
                     f ? f + 1 : bor_sites[best].file, bor_sites[best].line,
                     (unsigned long long)bor_sites[best].calls,
                     (double)bor_sites[best].us / 1000.0,
                     (unsigned long long)(bor_sites[best].bytes / 1024));
        }
    }
    if(bor_site_dropped)
    {
        prof_log("      (%llu allocations not attributed -- table full)",
                 (unsigned long long)bor_site_dropped);
    }
    memset(bor_sites, 0, sizeof(bor_sites));
    bor_site_dropped = 0;
}
#endif
