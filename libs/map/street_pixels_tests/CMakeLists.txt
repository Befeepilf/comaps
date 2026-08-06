project(street_pixels_tests)

set(SRC
  archive_tests.cpp
  assignment_persist_tests.cpp
  backend_config_tests.cpp
  collection_gate_tests.cpp
  collection_radius_tests.cpp
  country_config_tests.cpp
  eligibility_tests.cpp
  ever_live_tests.cpp
  explorer_pro_tests.cpp
  exploration_multiplier_tests.cpp
  interrupted_session_tests.cpp
  live_sample_acceptance_filter_tests.cpp
  pause_resume_semantics_tests.cpp
  recording_session_tests.cpp
  rematch_tests.cpp
  sample_acceptance_manager_tests.cpp
  segment_interpolation_tests.cpp
  street_pixel_tests.cpp
  street_pixels_file_tests.cpp
  street_pixels_test_helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    map
    drape_frontend
    routing
)
