project(street_pixels_config)

set(SRC
  country_config.cpp
  country_config.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    coding
  PRIVATE
    cppjansson
)
