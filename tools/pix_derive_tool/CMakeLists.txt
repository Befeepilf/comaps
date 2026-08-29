project(pix_derive_tool)

set(SRC pix_derive_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    map
    indexer
    platform
    geometry
    coding
    base
    gflags::gflags
)
