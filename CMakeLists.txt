cmake_minimum_required(VERSION 3.1)
#set(CMAKE_VERBOSE_MAKEFILE ON)

project(OpenBOR)
# Version hbmenu shows, and the name of the release archive.
#
# Derived from the repository rather than written out: a literal names the
# build it was typed at and goes stale on the next commit. The build number is
# the commit count, the same one engine/version.sh bakes into the engine's own
# version string, so the launcher and the engine always agree.
#
# The NACP carries the display version in 16 bytes, so 15 characters is all
# hbmenu will ever show and anything longer is silently cut.
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
if (OPENBOR_BUILD AND OPENBOR_COMMIT)
    set(OPENBOR_VERSION "3.0.${OPENBOR_BUILD}-${OPENBOR_COMMIT}")
    string(LENGTH "${OPENBOR_VERSION}" OPENBOR_VERSION_LENGTH)
    if (OPENBOR_VERSION_LENGTH GREATER 15)
        string(SUBSTRING "${OPENBOR_VERSION}" 0 15 OPENBOR_VERSION)
    endif ()
else ()
    set(OPENBOR_VERSION "3.0")
endif ()
message(STATUS "OpenBOR version: ${OPENBOR_VERSION}")

#####################
# OPTIONS
#####################
option(BUILD_GFX "Build with GFX support" ON)
option(BUILD_VORBIS "Build with BUILD_VORBIS support" ON)

##############
# COMMON
##############
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_GFX REQUIRED SDL2_gfx)
pkg_search_module(VPX REQUIRED vpx)
pkg_search_module(PNG REQUIRED libpng)
pkg_search_module(ZLIB REQUIRED zlib)

set(BOR_INC
        engine
        engine/sdl
        engine/source
        engine/source/adpcmlib
        engine/source/gamelib
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

set(BOR_CFLAGS -g -Wall -fsigned-char -std=gnu99 -fcommon -fomit-frame-pointer -DSDL -DSDL2 -DWEBM -DOPENGL -DLOADGL -D__CMAKE__ -D_FILE_OFFSET_BITS=64)
set(BOR_LDFLAGS ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARIES} ${VPX_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} m)

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
else ()
    #####################
    # LINUX PLATORM
    #####################
    list(APPEND BOR_CFLAGS -D__LINUX__ -DLINUX)
endif ()

#####################
# OpenBOR executable
#####################
add_executable(${CMAKE_PROJECT_NAME} ${BOR_SRC})
add_dependencies(${CMAKE_PROJECT_NAME} version_h)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${BOR_INC})
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${BOR_CFLAGS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${BOR_LDFLAGS})

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
