project(street_pixels_tests)

set(SRC
  exploration_multiplier_tests.cpp
  healpix_base_for_tests.cpp
  street_pixel_tests.cpp
  street_pixels_test_helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC} NO_PLATFORM_INIT)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    drape_frontend
    routing
)
