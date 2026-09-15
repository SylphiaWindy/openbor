/*
 * autoplay.c -- scripted input for profiling runs.  See autoplay.h.
 *
 * Diagnostic scaffolding, not part of the engine proper.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "autoplay.h"

#ifdef BOR_PROF

#include "openbor.h"
#include "prof.h"

enum { AP_WAIT, AP_TAP, AP_MASH, AP_QUIT };

typedef struct
{
    int kind;
    unsigned key;   /* FLAG_*, valid for AP_TAP / AP_MASH */
    int ms;
} ap_step;

/* Player 1's state, sampled once a second while a script runs.  Without this
   a stuck script is indistinguishable from input that never arrives. */
extern s_player player[];
extern u32 _time;
static uint64_t ap_last_probe = 0;

static void ap_probe(uint64_t now, unsigned emitted)
{
    entity *e;

    if(now - ap_last_probe < 300000)
    {
        return;
    }
    ap_last_probe = now;

    e = player[0].ent;
    prof_log("autoplay: emit=0x%04x keys=0x%04x ent=%s%s",
             emitted,
             (unsigned)player[0].keys,
             e ? "yes" : "NO",
             e ? "" : " (player has not joined)");
    if(e)
    {
        prof_log("autoplay:   pos x=%.1f z=%.1f a=%.1f '%s' spd=%.2f "
                 "movetime=%u _time=%u %s disable=0x%04x idle=%d",
                 e->position.x, e->position.z, e->position.y,
                 e->name ? e->name : "?",
                 e->modeldata.speed,
                 (unsigned)e->movetime, (unsigned)_time,
                 (e->movetime < _time) ? "MOVE-OK" : "MOVE-BLOCKED",
                 (unsigned)player[0].disablekeys,
                 e->idling);
    }
}

static ap_step *steps = NULL;
static int      step_count = 0;
static int      cursor = -1;        /* -1 = inactive */
static uint64_t step_started = 0;   /* us */

static const struct { const char *name; unsigned flag; } ap_keys[] =
{
    { "UP",       FLAG_MOVEUP    },
    { "DOWN",     FLAG_MOVEDOWN  },
    { "LEFT",     FLAG_MOVELEFT  },
    { "RIGHT",    FLAG_MOVERIGHT },
    { "ATTACK",   FLAG_ATTACK    },
    { "ATTACK2",  FLAG_ATTACK2   },
    { "ATTACK3",  FLAG_ATTACK3   },
    { "ATTACK4",  FLAG_ATTACK4   },
    { "JUMP",     FLAG_JUMP      },
    { "SPECIAL",  FLAG_SPECIAL   },
    { "START",    FLAG_START     },
    { "ESC",      FLAG_ESC       },
};

static long ap_lookup_key(const char *name)
{
    size_t i;
    for(i = 0; i < sizeof(ap_keys) / sizeof(ap_keys[0]); i++)
    {
        if(stricmp(name, ap_keys[i].name) == 0)
        {
            return (long)ap_keys[i].flag;
        }
    }
    return -1;
}

static void ap_push(int kind, unsigned key, int ms)
{
    steps = realloc(steps, sizeof(*steps) * (step_count + 1));
    if(!steps)
    {
        step_count = 0;
        return;
    }
    steps[step_count].kind = kind;
    steps[step_count].key  = key;
    steps[step_count].ms   = ms;
    step_count++;
}

void autoplay_init(void)
{
    const char *path = getenv("BOR_AUTOPLAY");
    FILE *fp;
    char line[256];
    int lineno = 0;

    if(!path || !path[0])
    {
        return;
    }

    fp = fopen(path, "r");
    if(!fp)
    {
        printf("autoplay: cannot open '%s', running unattended input disabled\n", path);
        return;
    }

    while(fgets(line, sizeof(line), fp))
    {
        char verb[64], arg[64];
        int ms;
        char *hash;

        lineno++;
        hash = strchr(line, '#');
        if(hash)
        {
            *hash = '\0';
        }

        verb[0] = arg[0] = '\0';
        ms = -1;
        if(sscanf(line, "%63s %63s %d", verb, arg, &ms) < 1)
        {
            continue;
        }

        if(stricmp(verb, "wait") == 0)
        {
            ap_push(AP_WAIT, 0, arg[0] ? atoi(arg) : 0);
        }
        else if(stricmp(verb, "tap") == 0)
        {
            long key = ap_lookup_key(arg);
            if(key < 0)
            {
                printf("autoplay: %s:%d unknown key '%s'\n", path, lineno, arg);
                continue;
            }
            ap_push(AP_TAP, (unsigned)key, ms > 0 ? ms : 120);
        }
        else if(stricmp(verb, "mash") == 0)
        {
            long key = ap_lookup_key(arg);
            if(key < 0)
            {
                printf("autoplay: %s:%d unknown key '%s'\n", path, lineno, arg);
                continue;
            }
            ap_push(AP_MASH, (unsigned)key, ms > 0 ? ms : 5000);
        }
        else if(stricmp(verb, "quit") == 0)
        {
            ap_push(AP_QUIT, 0, 0);
        }
        else
        {
            printf("autoplay: %s:%d unknown verb '%s'\n", path, lineno, verb);
        }
    }
    fclose(fp);

    if(step_count > 0)
    {
        cursor = 0;
        step_started = prof_us();
        prof_log("autoplay: %d step(s) from %s", step_count, path);
    }
}

unsigned long long autoplay_update(void)
{
    uint64_t now;
    double elapsed;

    if(cursor < 0 || cursor >= step_count)
    {
        return 0;
    }

    now = prof_us();
    elapsed = (double)(now - step_started) / 1000.0;

    switch(steps[cursor].kind)
    {
    case AP_WAIT:
        if(elapsed >= steps[cursor].ms)
        {
            break;
        }
        ap_probe(now, 0);
        return 0;

    case AP_TAP:
        if(elapsed < steps[cursor].ms)
        {
            ap_probe(now, steps[cursor].key);
            return steps[cursor].key;
        }
        /* Released; fall through to advance.  One idle frame between steps
           keeps two consecutive taps of the same key from merging. */
        break;

    case AP_MASH:
        /* Scene pages and menus each swallow one press, and how many there are
           is not knowable from here -- so hammer the key for a fixed span.
           140 ms on, 140 ms off: slow enough that every press is seen as a
           fresh newkey, fast enough to clear a long intro. */
        if(elapsed < steps[cursor].ms)
        {
            unsigned m = ((int)(elapsed / 140.0) % 2) ? 0u : steps[cursor].key;
            ap_probe(now, m);
            return m;
        }
        break;

    case AP_QUIT:
        /* borShutdown keeps pumping input while it tears down, so retire the
           script first or it re-enters this step on every teardown frame. */
        cursor = step_count;
        prof_log("autoplay: script finished, shutting down");
        prof_flush("autoplay done");
        borShutdown(0, "Autoplay script finished.\n");
        return 0;
    }

    cursor++;
    step_started = now;
    if(cursor < step_count)
    {
        prof_log("autoplay: step %d/%d", cursor + 1, step_count);
    }
    return 0;
}

#endif
