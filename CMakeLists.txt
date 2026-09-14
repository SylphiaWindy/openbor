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
