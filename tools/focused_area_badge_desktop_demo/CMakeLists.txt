project(focused_area_badge_desktop_demo)

set(SRC focused_area_badge_desktop_demo.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

find_package(Qt6 REQUIRED COMPONENTS Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Qt6::Widgets
    map
    street_pixels_areas
    street_pixels_config
    platform
    geometry
    base
    indexer
)

# Reuse PixelIdForLatLon helpers from the unit-test header (header-only).
target_include_directories(${PROJECT_NAME} PRIVATE ${OMIM_ROOT}/libs/map)
