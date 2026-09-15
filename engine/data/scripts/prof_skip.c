// PROFILING OVERRIDE -- loose file, not part of the mod.
//
// Ends the level a few seconds after it starts, so an unattended run can cross
// several level boundaries.  The point of the run is what a level load and
// unload cost, and how much the next level reuses; actually playing a stage
// through would take minutes and would not be reproducible.
//
// Every spawn entry in the level file is left alone, so the set of models --
// and therefore the set of sprites -- is exactly what a real playthrough loads.

void main()
{
    int t = getglobalvar("prof_skip_ticks");

    if(t == NULL()) { t = 0; }
    t = t + 1;
    setglobalvar("prof_skip_ticks", t);

    if(t > 180)                 // ~3 s at 60 ticks
    {
        setglobalvar("prof_skip_ticks", 0);
        finishlevel();
    }
}
