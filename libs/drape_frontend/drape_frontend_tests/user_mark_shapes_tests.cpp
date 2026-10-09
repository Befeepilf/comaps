#include "testing/testing.hpp"

#include "drape_frontend/user_mark_shapes.hpp"

#include "geometry/point2d.hpp"
#include "geometry/triangle2d.hpp"

#include <vector>

namespace user_mark_shapes_tests
{
bool IsCovered(std::vector<m2::PointD> const & triangles, m2::PointD const & pt)
{
  for (size_t i = 0; i + 2 < triangles.size(); i += 3)
    if (m2::IsPointInsideTriangle(pt, triangles[i], triangles[i + 1], triangles[i + 2]))
      return true;
  return false;
}

UNIT_TEST(TessellateRings_SquareWithHole)
{
  std::vector<std::vector<m2::PointD>> const rings = {
      {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}},
      {{4, 4}, {6, 4}, {6, 6}, {4, 6}, {4, 4}},
  };

  auto const triangles = df::TessellateRings(rings);

  TEST(!triangles.empty(), ());
  TEST_EQUAL(triangles.size() % 3, 0, ());
  TEST(IsCovered(triangles, {2, 2}), ());
  TEST(IsCovered(triangles, {8, 8}), ());
  TEST(IsCovered(triangles, {5, 3}), ());
  TEST(!IsCovered(triangles, {5, 5}), ());
}

UNIT_TEST(TessellateRings_IslandInsideHole)
{
  std::vector<std::vector<m2::PointD>> const rings = {
      {{0, 0}, {10, 0}, {10, 10}, {0, 10}},
      {{2, 2}, {8, 2}, {8, 8}, {2, 8}},
      {{4, 4}, {6, 4}, {6, 6}, {4, 6}},
  };

  auto const triangles = df::TessellateRings(rings);

  TEST(IsCovered(triangles, {1, 1}), ());
  TEST(!IsCovered(triangles, {3, 3}), ());
  TEST(IsCovered(triangles, {5, 5}), ());
}

UNIT_TEST(TessellateRings_DegenerateRings)
{
  std::vector<std::vector<m2::PointD>> const rings = {
      {{0, 0}, {1, 1}},
      {{0, 0}, {1, 1}, {0, 0}},
  };

  TEST(df::TessellateRings(rings).empty(), ());
}
}  // namespace user_mark_shapes_tests
