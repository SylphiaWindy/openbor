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
