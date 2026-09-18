# Everything on the Switch links statically, so SDL2's own dependencies have to
# follow it on the command line. This module is included after the engine's
# libraries are already on the link line, which is exactly where they belong.
target_link_libraries(${PROJECT_NAME} PUBLIC
  SDL2_gfx
  EGL
  glapi
  drm_nouveau
  nx
  stdc++
  m
)

# DevkitPro
#
# The NRO is named after the game this engine ships with, not the project:
# every release branch is an OpenBOR, and a release carries all of them side
# by side. hbmenu shows the NACP name.
set(SWITCH_NRO_NAME "SoRX")

nx_generate_nacp(${SWITCH_NRO_NAME}.nacp
  NAME    "${SWITCH_NRO_NAME}"
  AUTHOR  "cpasjuste, Sylphia"
  VERSION "${OPENBOR_VERSION}"
)

nx_create_nro(${PROJECT_NAME}
  OUTPUT ${SWITCH_NRO_NAME}.nro
  NACP   ${SWITCH_NRO_NAME}.nacp
  ICON   "${CMAKE_SOURCE_DIR}/engine/resources/switch_icon.jpg"
)

# Distribution Preperation
#
# The engine's own POST_BUILD step copies the text files into
# engine/releases, which a fresh checkout does not have, so make the tree
# here first: POST_BUILD commands run in the order they are added.
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/SWITCH/Logs
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/SWITCH/Paks
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/SWITCH/Saves
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/SWITCH/ScreenShots
)

# The NRO is built from the ELF by a command of its own, so it does not exist
# yet while the executable's POST_BUILD steps run. Copy it from a target that
# waits for it.
add_custom_target(${PROJECT_NAME}_nro_release ALL
  DEPENDS ${SWITCH_NRO_NAME}.nro
  COMMAND ${CMAKE_COMMAND} -E copy ${SWITCH_NRO_NAME}.nro ../engine/releases/SWITCH/${SWITCH_NRO_NAME}.nro
)

add_custom_target(${PROJECT_NAME}_switch_release
  DEPENDS ${SWITCH_NRO_NAME}.nro
  COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/${SWITCH_NRO_NAME}-${OPENBOR_VERSION}_switch.zip
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${SWITCH_NRO_NAME}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${SWITCH_NRO_NAME}.nro ${CMAKE_BINARY_DIR}/release/${SWITCH_NRO_NAME}/
  COMMAND cd ${CMAKE_BINARY_DIR}/release && zip -r ../${SWITCH_NRO_NAME}-${OPENBOR_VERSION}_switch.zip ${SWITCH_NRO_NAME}
)
