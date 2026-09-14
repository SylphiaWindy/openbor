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
nx_generate_nacp(${PROJECT_NAME}.nacp
  NAME    "${PROJECT_NAME}"
  AUTHOR  "cpasjuste, Sylphia"
  VERSION "${OPENBOR_VERSION}"
)

nx_create_nro(${PROJECT_NAME}
  NACP ${PROJECT_NAME}.nacp
  ICON "${CMAKE_SOURCE_DIR}/engine/resources/switch_icon.jpg"
)

# Distribution Preperation
add_custom_target(${PROJECT_NAME}_switch_release
  DEPENDS ${PROJECT_NAME}.nro
  COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${OPENBOR_VERSION}_switch.zip
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
  COMMAND cd ${CMAKE_BINARY_DIR}/release && zip -r ../${PROJECT_NAME}-${OPENBOR_VERSION}_switch.zip ${PROJECT_NAME}
)
