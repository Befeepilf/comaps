project(street_pixels_tests)

set(SRC
  backend_config_tests.cpp
  exploration_multiplier_tests.cpp
  street_pixel_tests.cpp
  street_pixels_test_helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    map
    drape_frontend
    routing
)
