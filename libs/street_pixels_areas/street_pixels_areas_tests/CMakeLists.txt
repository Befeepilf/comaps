project(street_pixels_areas_tests)

set(SRC
  area_completion_cache_tests.cpp
  area_milestone_store_tests.cpp
  area_overlay_tests.cpp
  city_completion_cache_tests.cpp
  exploration_area_resolver_tests.cpp
  exploration_filter_tests.cpp
  exploration_sidecar_tests.cpp
  exploration_area_tap_tests.cpp
  live_recency_store_tests.cpp
  ownership_scoring_tests.cpp
  focus_selection_engine_tests.cpp
  sparse_assignment_store_tests.cpp
  spa_dense_emit_tests.cpp
  spa_jsonl_emit_tests.cpp
  spa_serdes_tests.cpp
  settlement_containment_tests.cpp
  subdivision_assigner_tests.cpp
  subdivision_assignment_tests.cpp
  test_helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    street_pixels_areas
)
