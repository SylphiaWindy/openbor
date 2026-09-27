cmake_minimum_required(VERSION 3.1)
#set(CMAKE_VERBOSE_MAKEFILE ON)

project(FFLNS)
# Version hbmenu shows, and the name of the release archive.
#
# Derived from the repository rather than written out, because a literal goes
# stale the moment anything is committed: the old one said 7127 while the tree
# it names was 7144 and climbing. The build number is the commit count, the
# same number engine/version.sh bakes into the engine's own version string, so
# the two always agree.
find_package(Git QUIET)
if (GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    # Re-run configure when the checkout moves, or the version below is read
    # once and then repeated by every later build.
    #
    # HEAD alone is not enough: on a branch it holds "ref: refs/heads/name" and
    # committing does not touch it, only the ref it names. Track both. A packed
    # ref has no file to watch, and then only branch switches are noticed.
    #
    # In a linked worktree .git is a file pointing at the real directory, and
    # the per-worktree HEAD sits apart from the shared refs, so ask git for
    # both paths rather than assuming either.
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE OPENBOR_GIT_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --path-format=absolute --git-common-dir
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE OPENBOR_GIT_COMMON_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    if (NOT OPENBOR_GIT_COMMON_DIR)
        set(OPENBOR_GIT_COMMON_DIR "${OPENBOR_GIT_DIR}")
    endif ()
    if (EXISTS "${OPENBOR_GIT_DIR}/HEAD")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${OPENBOR_GIT_DIR}/HEAD")
        file(READ "${OPENBOR_GIT_DIR}/HEAD" OPENBOR_GIT_HEAD)
        string(STRIP "${OPENBOR_GIT_HEAD}" OPENBOR_GIT_HEAD)
        if (OPENBOR_GIT_HEAD MATCHES "^ref: (.+)$" AND EXISTS "${OPENBOR_GIT_COMMON_DIR}/${CMAKE_MATCH_1}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                    "${OPENBOR_GIT_COMMON_DIR}/${CMAKE_MATCH_1}")
        endif ()
    endif ()

    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE OPENBOR_BUILD
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=6 HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE OPENBOR_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
endif ()
# The NACP carries the display version in 16 bytes, so 15 characters is all
# hbmenu will ever show; anything longer is silently cut. "FFLNS" does not
# belong in it either, hbmenu prints the name on the line above.
if (OPENBOR_BUILD AND OPENBOR_COMMIT)
    set(OPENBOR_VERSION "3.0.${OPENBOR_BUILD}-${OPENBOR_COMMIT}")
    string(LENGTH "${OPENBOR_VERSION}" OPENBOR_VERSION_LENGTH)
    if (OPENBOR_VERSION_LENGTH GREATER 15)
        string(SUBSTRING "${OPENBOR_VERSION}" 0 15 OPENBOR_VERSION)
    endif ()
else ()
    set(OPENBOR_VERSION "3.0-FFLNS")
endif ()
message(STATUS "OpenBOR version: ${OPENBOR_VERSION}")

#####################
# OPTIONS
#####################
option(BUILD_GFX "Build with GFX support" ON)
option(BUILD_VORBIS "Build with BUILD_VORBIS support" OFF)
option(BUILD_TREMOR "Build with TREMOR support" ON)
# The Windows SDK carries libvorbis and libogg but no Tremor, so the Windows
# build decodes with the floating-point library. soundmix.c and webmlib pick
# their headers off TREMOR, so flipping the two options is all it takes.
if (WIN32 AND BUILD_TREMOR)
    set(BUILD_TREMOR OFF)
    set(BUILD_VORBIS ON)
endif ()
##############
# COMMON
##############
include(FindPkgConfig)
pkg_search_module(SDL2_GFX REQUIRED SDL2_gfx)
pkg_search_module(VPX REQUIRED vpx)
pkg_search_module(PNG REQUIRED libpng)
pkg_search_module(ZLIB REQUIRED zlib)
pkg_search_module(SDL2 REQUIRED sdl2)

set(BOR_INC
        engine
        engine/sdl
        engine/source
        engine/source/adpcmlib
        engine/source/gamelib
        engine/source/gfxlib
        engine/source/openborscript
        engine/source/pnglib
        engine/source/preprocessorlib
        engine/source/ramlib
        engine/source/randlib
        engine/source/scriptlib
        engine/source/webmlib
        engine/source/webmlib/halloc
        engine/source/webmlib/nestegg
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIRS}
        ${VPX_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        )

file(GLOB BOR_SRC
        engine/*.c
        engine/sdl/*.c
        engine/source/*.c
        engine/source/adpcmlib/*.c
        engine/source/gamelib/*.c
        engine/source/gfxlib/*.c
        engine/source/openborscript/*.c
        engine/source/pnglib/*.c
        engine/source/preprocessorlib/*.c
        engine/source/ramlib/*.c
        engine/source/randlib/*.c
        engine/source/scriptlib/*.c
        engine/source/webmlib/*.c
        engine/source/webmlib/halloc/*.c
        engine/source/webmlib/nestegg/*.c
        )

set(BOR_CFLAGS -g -Wall -fsigned-char -std=gnu99 -fomit-frame-pointer -DSDL -DSDL2 -DWEBM -DOPENGL -DLOADGL -D__CMAKE__ -D_FILE_OFFSET_BITS=64)
set(BOR_LDFLAGS ${SDL2_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${VPX_LIBRARIES} m)

##############
# VERSION.SH
##############
# version.h lands in the source tree, so a plain OUTPUT dependency never
# refires once the file exists and the build then reports a stale commit.
# Regenerate it on every build instead.
ADD_CUSTOM_TARGET(version_h ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/engine
        COMMAND bash version.sh
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/engine/version.h)

#####################
# OPTIONS
#####################
if (BUILD_GFX)
    list(APPEND BOR_INC engine/source/gfxlib)
    file(GLOB GFX_SRC engine/source/gfxlib/*.c)
    list(APPEND BOR_SRC ${GFX_SRC})
endif ()
if (BUILD_VORBIS)
    pkg_search_module(VORBIS REQUIRED vorbis)
    pkg_search_module(VORBISFILE REQUIRED vorbisfile)
    pkg_search_module(OGG REQUIRED ogg)
    list(APPEND BOR_INC ${VORBIS_INCLUDE_DIRS} ${VORBISFILE_INCLUDE_DIRS} ${OGG_INCLUDE_DIRS})
    list(APPEND BOR_CFLAGS -DOV_EXCLUDE_STATIC_CALLBACKS)
    list(APPEND BOR_LDFLAGS ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES} ${OGG_LIBRARIES} m)
endif ()
if (BUILD_TREMOR)
    list(APPEND BOR_CFLAGS -DTREMOR)
    list(APPEND BOR_LDFLAGS vorbisidec ogg)
endif (BUILD_TREMOR)
#####################
# PLATORM SPECIFIC
#####################
if (NINTENDO_SWITCH)
    #####################
    # SWITCH PLATORM
    #####################
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND BOR_CFLAGS -D__NXLINK__)
    endif ()
    list(APPEND BOR_LDFLAGS stdc++)
elseif (WIN32)
    #####################
    # WINDOWS PLATORM
    #####################
    # Cross-built from macOS with mingw-w64 against the SDK that ships in
    # tools/win-sdk; see openbor-tools/mingw32-toolchain.cmake. The engine's
    # Windows paths key off WIN, and ELF is what the other Windows builds
    # define alongside it.
    list(APPEND BOR_CFLAGS -DWIN -DELF)
    list(APPEND BOR_LDFLAGS opengl32 winmm gdi32 ole32 oleaut32 imm32 version
            setupapi psapi hid shell32 user32 uuid dinput8 mingw32)
    # Without this the linker produces a console subsystem binary and Windows
    # opens a terminal alongside the game.
    list(APPEND BOR_LDFLAGS -mwindows)

    # Icon and version info. One tree carries a .rc to compile, the other a
    # .res already built; without either the exe gets Windows' blank default.
    set(BOR_RC  "${CMAKE_SOURCE_DIR}/engine/resources/OpenBOR.rc")
    set(BOR_RES "${CMAKE_SOURCE_DIR}/engine/resources/OpenBOR.res")
    if (EXISTS "${BOR_RC}")
        set(BOR_RES "${CMAKE_BINARY_DIR}/OpenBOR.res")
        add_custom_command(OUTPUT ${BOR_RES}
                COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_SOURCE_DIR}/engine
                        -O coff -o ${BOR_RES} OpenBOR.rc
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/engine/resources
                DEPENDS ${BOR_RC}
                COMMENT "Compiling Windows resources")
        add_custom_target(openbor_resources DEPENDS ${BOR_RES})
        # The .rc includes version.h, which version_h writes; without this
        # a parallel build from a clean tree compiles the resource first.
        add_dependencies(openbor_resources version_h)
        set(BOR_RES_TARGET openbor_resources)
    elseif (NOT EXISTS "${BOR_RES}")
        unset(BOR_RES)
    endif ()
    list(APPEND BOR_LINK_DIRS_WANTED 1)
elseif (APPLE)
    #####################
    # MACOS PLATORM
    #####################
    # Mac rides on the Linux paths -- ram.c and packfile.c both branch on
    # LINUX -- with DARWIN on top for the few places that want the Mach or BSD
    # call instead of the Linux one.
    list(APPEND BOR_CFLAGS -D__LINUX__ -DLINUX -DDARWIN)

    # <malloc.h> is the Linux spelling; the SDK keeps that header a directory
    # down, so put the directory on the search path rather than touching every
    # include of it.
    execute_process(COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE MACOS_SDK_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    if (MACOS_SDK_PATH AND EXISTS "${MACOS_SDK_PATH}/usr/include/malloc")
        list(APPEND BOR_INC ${MACOS_SDK_PATH}/usr/include/malloc)
    endif ()

    foreach (FRAMEWORK Cocoa OpenGL Carbon AudioUnit IOKit)
        find_library(${FRAMEWORK}_FRAMEWORK ${FRAMEWORK} REQUIRED)
        list(APPEND BOR_LDFLAGS ${${FRAMEWORK}_FRAMEWORK})
    endforeach ()
    list(APPEND BOR_LINK_DIRS_WANTED 1)
else ()
    #####################
    # LINUX PLATORM
    #####################
    list(APPEND BOR_CFLAGS -D__LINUX__ -DLINUX)
endif ()

# pkg-config reports bare library names; where they do not sit in a default
# directory -- a Homebrew prefix, the unpacked Windows SDK -- the paths it
# reports alongside them have to be passed on too.
if (BOR_LINK_DIRS_WANTED)
    link_directories(
            ${SDL2_LIBRARY_DIRS} ${SDL2_GFX_LIBRARY_DIRS} ${VPX_LIBRARY_DIRS}
            ${PNG_LIBRARY_DIRS} ${ZLIB_LIBRARY_DIRS}
            ${VORBIS_LIBRARY_DIRS} ${VORBISFILE_LIBRARY_DIRS} ${OGG_LIBRARY_DIRS})
endif ()

#####################
# OpenBOR executable
#####################
add_executable(${CMAKE_PROJECT_NAME} ${BOR_SRC})
add_dependencies(${CMAKE_PROJECT_NAME} version_h)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${BOR_INC})
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${BOR_CFLAGS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${BOR_LDFLAGS})
if (BOR_RES)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "${BOR_RES}")
    if (BOR_RES_TARGET)
        add_dependencies(${CMAKE_PROJECT_NAME} ${BOR_RES_TARGET})
    endif ()
endif ()

#####################
# SWITCH target
#####################
if (NINTENDO_SWITCH)
   nx_generate_nacp(${CMAKE_PROJECT_NAME}.nacp
           NAME "${CMAKE_PROJECT_NAME}"
           AUTHOR "cpasjuste, Sylphia"
           VERSION "${OPENBOR_VERSION}"
           )
   nx_create_nro(${CMAKE_PROJECT_NAME}
           NACP ${CMAKE_PROJECT_NAME}.nacp
           ICON "${CMAKE_SOURCE_DIR}/engine/resources/switch_icon.jpg"
           )
   add_custom_target(${PROJECT_NAME}_switch_release
           DEPENDS ${PROJECT_NAME}.nro
           COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${OPENBOR_VERSION}_switch.zip
           COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
           COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
           COMMAND cd ${CMAKE_BINARY_DIR}/release && zip -r ../${PROJECT_NAME}-${OPENBOR_VERSION}_switch.zip ${PROJECT_NAME}
          )
endif ()
