if(NOT NINTENDO_SWITCH)
  message(NOTICE "Invoke DevkitPro's cmake utility directly:")
  message(NOTICE "\t$ENV{DEVKITPRO}/portlibs/switch/bin/aarch64-none-elf-cmake -DBUILD_SWITCH=ON ..\n")
endif()

# devkitA64's GCC is newer than the one upstream builds with, and Release turns
# every warning it has gained since into an error.
set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-maybe-uninitialized -Wno-stringop-truncation -Wno-enum-int-mismatch -Wno-array-bounds -Wno-stringop-overflow -Wno-address -Wno-format-truncation")

set(USE_SDL     ON)
set(USE_OPENGL  ON)
set(USE_LOADGL  ON)
set(USE_GFX     ON)
set(USE_VORBIS  ON)
set(USE_WEBM    ON)

add_definitions(
  -DSDL2
  -D__CMAKE__
  -D_FILE_OFFSET_BITS=64
)

# Routes stdout to nxlink so a debug build can be watched from the host.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-D__NXLINK__)
endif()

# The version hbmenu shows, and the name of the release archive.
#
# Derived from the repository rather than written out: a literal names the
# build it was typed at and goes stale on the next commit. The build number is
# the commit count, the same one engine/version.sh bakes into the engine's own
# version string, so the launcher and the engine always agree.
#
# The NACP carries the display version in 16 bytes, so 15 characters is all
# hbmenu will ever show and anything longer is silently cut.
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
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
  if(NOT OPENBOR_GIT_COMMON_DIR)
    set(OPENBOR_GIT_COMMON_DIR "${OPENBOR_GIT_DIR}")
  endif()
  if(EXISTS "${OPENBOR_GIT_DIR}/HEAD")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${OPENBOR_GIT_DIR}/HEAD")
    file(READ "${OPENBOR_GIT_DIR}/HEAD" OPENBOR_GIT_HEAD)
    string(STRIP "${OPENBOR_GIT_HEAD}" OPENBOR_GIT_HEAD)
    if(OPENBOR_GIT_HEAD MATCHES "^ref: (.+)$" AND EXISTS "${OPENBOR_GIT_COMMON_DIR}/${CMAKE_MATCH_1}")
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
              "${OPENBOR_GIT_COMMON_DIR}/${CMAKE_MATCH_1}")
    endif()
  endif()

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
endif()
if(OPENBOR_BUILD AND OPENBOR_COMMIT)
  set(OPENBOR_VERSION "4.0.${OPENBOR_BUILD}-${OPENBOR_COMMIT}")
  string(LENGTH "${OPENBOR_VERSION}" OPENBOR_VERSION_LENGTH)
  if(OPENBOR_VERSION_LENGTH GREATER 15)
    string(SUBSTRING "${OPENBOR_VERSION}" 0 15 OPENBOR_VERSION)
  endif()
else()
  set(OPENBOR_VERSION "4.0")
endif()
message(STATUS "OpenBOR version: ${OPENBOR_VERSION}")
