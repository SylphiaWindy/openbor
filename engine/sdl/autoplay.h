/*
 * autoplay.h -- scripted input for profiling runs.
 *
 * Diagnostic scaffolding, not part of the engine proper.  Compiled only when
 * BOR_PROF is defined, and inert unless the environment variable BOR_AUTOPLAY
 * names a script file.
 *
 * The point is reproducibility: a profiling run has to walk the same path
 * through title -> select -> level every time, and a human cannot do that.
 * Input is injected at the same place a real pad would land (the key mask in
 * control_update), so every code path -- menus included -- runs for real.
 *
 * Script syntax, one action per line, '#' starts a comment:
 *
 *     wait <ms>              do nothing for <ms>
 *     tap  <KEY> [<ms>]      hold <KEY> for <ms> (default 120), then release
 *     mash <KEY> [<ms>]      tap <KEY> repeatedly for <ms> (default 5000);
 *                            use it to walk through scenes and menus whose
 *                            page count is not known in advance
 *     quit                   flush the profile and shut down cleanly
 *
 * <KEY> names a FLAG_* button without the prefix: UP DOWN LEFT RIGHT ATTACK
 * ATTACK2 ATTACK3 ATTACK4 JUMP SPECIAL START ESC.
 */
#ifndef BOR_AUTOPLAY_H
#define BOR_AUTOPLAY_H

#ifdef BOR_PROF

/* Reads $BOR_AUTOPLAY, if set.  Safe to call when it is not. */
void autoplay_init(void);

/* The key mask player 1 should see this frame; 0 when inactive or finished. */
unsigned long long autoplay_update(void);

#endif
#endif
