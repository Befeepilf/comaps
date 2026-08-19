project(street_pixels_areas)

set(SRC
  area_completion_cache.cpp
  area_completion_cache.hpp
  area_milestone_store.cpp
  area_milestone_store.hpp
  area_overlay.cpp
  area_overlay.hpp
  areas_format.hpp
  areas_reader.cpp
  areas_reader.hpp
  areas_serdes.cpp
  areas_serdes.hpp
  areas_serdes_impl.hpp
  areas_types.hpp
  areas_writer.cpp
  areas_writer.hpp
  bg_point.hpp
  city_completion_cache.cpp
  city_completion_cache.hpp
  exploration_area_resolver.cpp
  exploration_area_resolver.hpp
  exploration_filter.cpp
  exploration_filter.hpp
  exploration_sidecar.cpp
  exploration_sidecar.hpp
  exploration_area_tap.hpp
  focused_area_progress.hpp
  focus_selection_engine.cpp
  focus_selection_engine.hpp
  sample_centres.cpp
  sample_centres.hpp
  settlement_containment.cpp
  settlement_containment.hpp
  sparse_assignment_store.cpp
  sparse_assignment_store.hpp
  spa_jsonl.cpp
  spa_jsonl.hpp
  subdivision_assigner.cpp
  subdivision_assigner.hpp
  subdivision_assignment.cpp
  subdivision_assignment.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

# Light HEALPix centres via chealpix C (avoids linking full cxx healpix / libsharp).
target_sources(${PROJECT_NAME} PRIVATE
  ${OMIM_ROOT}/3party/healpix/healpix/src/C/subs/chealpix.c
)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${OMIM_ROOT}/3party/healpix/healpix/src/C/subs
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    coding
    geometry
    platform
    street_pixels_config
  PRIVATE
    cppjansson
    sqlite
)

omim_add_test_subdirectory(street_pixels_areas_tests)
