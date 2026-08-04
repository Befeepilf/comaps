project(street_pixels_areas_tests)

set(SRC
  exploration_filter_tests.cpp
  exploration_sidecar_tests.cpp
  spa_serdes_tests.cpp
  subdivision_assigner_tests.cpp
  test_helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    street_pixels_areas
)
