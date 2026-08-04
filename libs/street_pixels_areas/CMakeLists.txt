project(street_pixels_areas)

set(SRC
  areas_format.hpp
  areas_reader.cpp
  areas_reader.hpp
  areas_serdes.cpp
  areas_serdes.hpp
  areas_serdes_impl.hpp
  areas_types.hpp
  areas_writer.cpp
  areas_writer.hpp
  exploration_filter.cpp
  exploration_filter.hpp
  exploration_sidecar.cpp
  exploration_sidecar.hpp
  subdivision_assigner.cpp
  subdivision_assigner.hpp
  subdivision_assignment.cpp
  subdivision_assignment.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    coding
    geometry
    platform
    street_pixels_config
)

omim_add_test_subdirectory(street_pixels_areas_tests)
