/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

/* before the engine headers: safealloc.h macro-wraps malloc/free */
#include <malloc.h>

#include "sdlport.h"
#include "packfile.h"
#include "ram.h"
#include "video.h"
#include "menu.h"
#include "prof.h"
#include <time.h>
#include <unistd.h>

#undef usleep

#ifdef DARWIN
#include <CoreFoundation/CoreFoundation.h>
#elif WIN
#undef main
#elif defined(__SWITCH__) && defined(__NXLINK__)
#include <switch.h>
int nx_sock = -1;
#endif

char packfile[MAX_FILENAME_LEN] = {"bor.pak"};
#if ANDROID
#include <unistd.h>
char rootDir[MAX_BUFFER_LEN] = {""};
#endif
char paksDir[MAX_FILENAME_LEN] = {"Paks"};
char savesDir[MAX_FILENAME_LEN] = {"Saves"};
char logsDir[MAX_FILENAME_LEN] = {"Logs"};
char screenShotsDir[MAX_FILENAME_LEN] = {"ScreenShots"};

// sleeps for the given number of microseconds
#if _POSIX_C_SOURCE >= 199309L
void _usleep(u32 usec)
{
    struct timespec sleeptime;
    sleeptime.tv_sec = usec / 1000000LL;
    sleeptime.tv_nsec = (usec % 1000000LL) * 1000;
    nanosleep(&sleeptime, NULL);
}
#endif

#if ANDROID
char* AndroidRoot(char *relPath)
{
	static char filename[MAX_FILENAME_LEN];
	strcpy(filename, rootDir);
	strcat(filename, relPath);
	return filename;
}
#endif

void borExit(int reset)
{
#ifdef GP2X
	gp2x_end();
	chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#elif SDL
	SDL_Delay(1000);
	SDL_Quit(); // call this instead of atexit(SDL_Quit); It's best practice!
#endif

#if defined(__SWITCH__) && defined(__NXLINK__)
    if (nx_sock != -1)
        close(nx_sock);
    socketExit();
#endif

    exit(reset);
}

int main(int argc, char *argv[])
{
#ifndef SKIP_CODE
	char pakname[MAX_FILENAME_LEN] = {""};
#endif
#ifdef CUSTOM_SIGNAL_HANDLER
	struct sigaction sigact;
#endif

#ifdef DARWIN
	char resourcePath[PATH_MAX] = {""};
	CFBundleRef mainBundle;
	CFURLRef resourcesDirectoryURL;
	mainBundle = CFBundleGetMainBundle();
	resourcesDirectoryURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	if(!CFURLGetFileSystemRepresentation(resourcesDirectoryURL, true, (UInt8 *) resourcePath, PATH_MAX))
	{
		borExit(0);
	}
	CFRelease(resourcesDirectoryURL);
	chdir(resourcePath);
#endif

#ifdef CUSTOM_SIGNAL_HANDLER
	sigact.sa_sigaction = handleFatalSignal;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	if(sigaction(SIGSEGV, &sigact, NULL) != 0)
	{
		printf("Error setting signal handler for %d (%s)\n", SIGSEGV, strsignal(SIGSEGV));
		borExit(EXIT_FAILURE);
	}
#endif

#if defined(__SWITCH__) && defined(__NXLINK__)
    socketInitializeDefault();
    nx_sock = nxlinkStdio();
#endif

	PROF_T0(_p_boot);
	prof_log("main: entered");

#ifdef __SWITCH__
	/*
	 * Keep freed memory in the process instead of handing it back.
	 *
	 * dlmalloc trims the top of the heap once the free space there passes
	 * M_TRIM_THRESHOLD (128 KB by default), which on Switch means an
	 * svcSetHeapSize. Tearing down a level frees ~700k script instructions,
	 * and the frees that land near the top were measured at 12 us each
	 * against 0.19 us for the ones that do not -- a syscall apiece.
	 *
	 * The engine's peak footprint is the same either way; this only stops
	 * the heap oscillating while a level is being built up or torn down.
	 */
	/*
	 * Measured and reverted: raising the trim threshold changed nothing
	 * (13.6 s of teardown became 12.4 s, inside run-to-run variance), so the
	 * frees were never paying for svcSetHeapSize. All it did was stop the
	 * arena ever shrinking, and the arena is what the options screen reports
	 * as used memory. The cost was in the allocator's own bookkeeping, which
	 * the instruction pool now sidesteps.
	 */
	prof_log("main: entered (no mallopt; see comment)");
#endif

	setSystemRam();
	initSDL();
	prof_log("main: initSDL done, %.3f ms", PROF_SINCE(_p_boot));

	packfile_mode(0);

#ifdef ANDROID
    if(strstr(SDL_AndroidGetExternalStoragePath(), "org.openborfflns.engine"))
    {
        strcpy(rootDir, "/mnt/sdcard/OpenBOR/");
        strcpy(paksDir, "/mnt/sdcard/OpenBOR/Paks");
        strcpy(savesDir, "/mnt/sdcard/OpenBOR/Saves");
        strcpy(logsDir, "/mnt/sdcard/OpenBOR/Logs");
        strcpy(screenShotsDir, "/mnt/sdcard/OpenBOR/ScreenShots");
    }
    else
    {
        strcpy(rootDir, SDL_AndroidGetExternalStoragePath());
        strcat(rootDir, "/");
        strcpy(paksDir, SDL_AndroidGetExternalStoragePath());
        strcat(paksDir, "/Paks");
        strcpy(savesDir, SDL_AndroidGetExternalStoragePath());
        strcat(savesDir, "/Saves");
        strcpy(logsDir, SDL_AndroidGetExternalStoragePath());
        strcat(logsDir, "/Logs");
        strcpy(screenShotsDir, SDL_AndroidGetExternalStoragePath());
        strcat(screenShotsDir, "/ScreenShots");
    }
	dirExists(rootDir, 1);
    chdir(rootDir);
#endif

	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);
	prof_log("main: dirExists x4 done, %.3f ms", PROF_SINCE(_p_boot));

   // Test command line argument to launch MOD
   int romArg = 0;
   if(argc == 2) {
      memcpy(packfile, argv[1], strlen(argv[1]));
      if(fileExists(packfile)) {
         romArg = 1;
      }
   }

   if(!romArg) {
       Menu();
       prof_log("main: Menu() returned, %.3f ms since boot", PROF_SINCE(_p_boot));
       prof_flush("menu closed");
   }

#ifndef SKIP_CODE
	getPakName(pakname, -1);
	video_set_window_title(pakname);
#endif
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

