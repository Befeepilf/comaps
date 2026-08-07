project(spa_emit_tool)

set(SRC spa_emit_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    coding
    gflags::gflags
    geometry
    platform
    street_pixels_areas
    street_pixels_config
)
