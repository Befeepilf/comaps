#include "street_pixels_areas/completion_card.hpp"

#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "geometry/rect2d.hpp"

#include "platform/platform.hpp"

#include "3party/stb_image/stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace street_pixels
{
namespace
{
uint8_t constexpr kBackgroundR = 250;
uint8_t constexpr kBackgroundG = 250;
uint8_t constexpr kBackgroundB = 250;
uint8_t constexpr kBackgroundA = 255;
uint8_t constexpr kStrokeR = 30;
uint8_t constexpr kStrokeG = 30;
uint8_t constexpr kStrokeB = 30;
uint8_t constexpr kStrokeA = 255;

bool HasUsableRing(std::vector<std::vector<m2::PointD>> const & rings)
{
  for (auto const & ring : rings)
  {
    if (ring.size() >= 3)
      return true;
  }
  return false;
}

void AppendLabel(std::string & text, std::string const & part)
{
  if (part.empty())
    return;
  if (!text.empty())
    text.push_back(' ');
  text += part;
}

void PutPixel(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height, int x, int y)
{
  if (x < 0 || y < 0)
    return;
  uint32_t const ux = static_cast<uint32_t>(x);
  uint32_t const uy = static_cast<uint32_t>(y);
  if (ux >= width || uy >= height)
    return;
  size_t const i = (static_cast<size_t>(uy) * width + ux) * 4;
  rgba[i] = kStrokeR;
  rgba[i + 1] = kStrokeG;
  rgba[i + 2] = kStrokeB;
  rgba[i + 3] = kStrokeA;
}

void PlotBrush(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height, int x, int y)
{
  for (int dy = -1; dy <= 1; ++dy)
  {
    for (int dx = -1; dx <= 1; ++dx)
      PutPixel(rgba, width, height, x + dx, y + dy);
  }
}

void StrokeLine(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height, int x0, int y0, int x1, int y1)
{
  int const dx = std::abs(x1 - x0);
  int const sx = x0 < x1 ? 1 : -1;
  int const dy = -std::abs(y1 - y0);
  int const sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true)
  {
    PlotBrush(rgba, width, height, x0, y0);
    if (x0 == x1 && y0 == y1)
      break;
    int const e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void StrokeClosedRing(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height,
                      std::vector<m2::PointD> const & ring)
{
  if (ring.size() < 3)
    return;
  auto roundCoord = [](double v) { return static_cast<int>(std::lround(v)); };
  for (size_t i = 0; i < ring.size(); ++i)
  {
    m2::PointD const & a = ring[i];
    m2::PointD const & b = ring[(i + 1) % ring.size()];
    StrokeLine(rgba, width, height, roundCoord(a.x), roundCoord(a.y), roundCoord(b.x), roundCoord(b.y));
  }
}
}  // namespace

std::vector<std::string> CompletionCardPermittedKeys()
{
  return {"areaDisplayName", "headline", "outlineRings", "nickname", "completedDate", "branding", "competitionLine"};
}

std::vector<std::string> CompletionCardDeniedKeys()
{
  return {"route",          "track",         "home",         "liveLocation", "latitude",
          "longitude",      "lat",           "lon",          "geo:",         "ge0",
          "healpix",        "pixelId",       "mwmId",        "countryId",    "osmId",
          "compactIndex",   "userId",        "visitTimestamp", "positionMarker", "gps"};
}

std::vector<std::string> PresentFieldNames(CompletionCardModel const & model)
{
  std::vector<std::string> keys;
  keys.push_back("areaDisplayName");
  keys.push_back("headline");
  keys.push_back("outlineRings");
  if (model.m_nickname)
    keys.push_back("nickname");
  if (model.m_completedDate)
    keys.push_back("completedDate");
  keys.push_back("branding");
  keys.push_back("competitionLine");
  return keys;
}

std::string CompletionCardLabelText(CompletionCardModel const & model)
{
  std::string text;
  AppendLabel(text, model.m_areaDisplayName);
  AppendLabel(text, model.m_headline);
  AppendLabel(text, model.m_branding);
  AppendLabel(text, model.m_competitionLine);
  if (model.m_nickname)
    AppendLabel(text, *model.m_nickname);
  if (model.m_completedDate)
    AppendLabel(text, *model.m_completedDate);
  return text;
}

std::optional<CompletionCardModel> ComposeCompletionCard(CompletionCardSource const & source,
                                                         CompletionCardOptions const & options)
{
  std::string name = source.m_displayName;
  strings::Trim(name);
  if (name.empty() || !HasUsableRing(source.m_rings))
    return std::nullopt;

  CompletionCardModel model;
  model.m_areaDisplayName = std::move(name);
  model.m_headline = kCompletionCardHeadline;
  model.m_outlineRings = source.m_rings;
  model.m_branding = kCompletionCardBranding;
  model.m_competitionLine = source.m_competitionLine;

  if (options.nickname)
  {
    std::string nick = *options.nickname;
    strings::Trim(nick);
    if (!nick.empty())
      model.m_nickname = std::move(nick);
  }

  if (source.m_completed100At && *source.m_completed100At >= 0)
  {
    std::string const iso = base::SecondsSinceEpochToString(static_cast<uint64_t>(*source.m_completed100At));
    if (iso.size() >= 10)
      model.m_completedDate = iso.substr(0, 10);
  }

  return model;
}

std::vector<std::vector<m2::PointD>> ProjectOutlineToPixels(std::vector<std::vector<m2::PointD>> const & rings,
                                                            uint32_t width, uint32_t height, double padFraction)
{
  std::vector<std::vector<m2::PointD>> projected;
  if (width == 0 || height == 0)
    return projected;

  m2::RectD bounds;
  for (auto const & ring : rings)
  {
    for (auto const & p : ring)
      bounds.Add(p);
  }
  if (!bounds.IsValid())
    return projected;

  double const spanX = std::max(bounds.SizeX(), 1e-9);
  double const spanY = std::max(bounds.SizeY(), 1e-9);
  double const padX = spanX * padFraction;
  double const padY = spanY * padFraction;
  m2::RectD const padded(bounds.minX() - padX, bounds.minY() - padY, bounds.maxX() + padX, bounds.maxY() + padY);
  double const paddedW = std::max(padded.SizeX(), 1e-9);
  double const paddedH = std::max(padded.SizeY(), 1e-9);
  double const scale = std::min(static_cast<double>(width) / paddedW, static_cast<double>(height) / paddedH);
  double const usedW = paddedW * scale;
  double const usedH = paddedH * scale;
  double const originX = (static_cast<double>(width) - usedW) / 2.0;
  double const originY = (static_cast<double>(height) - usedH) / 2.0;

  projected.reserve(rings.size());
  for (auto const & ring : rings)
  {
    std::vector<m2::PointD> out;
    out.reserve(ring.size());
    for (auto const & p : ring)
    {
      double const x = originX + (p.x - padded.minX()) * scale;
      double const y = originY + (padded.maxY() - p.y) * scale;
      out.emplace_back(x, y);
    }
    projected.push_back(std::move(out));
  }
  return projected;
}

bool RasteriseCompletionCard(CompletionCardModel const & model, uint32_t width, uint32_t height,
                             std::vector<uint8_t> & rgba8888)
{
  if (width == 0 || height == 0)
    return false;
  auto const projected = ProjectOutlineToPixels(model.m_outlineRings, width, height);
  if (projected.empty())
    return false;

  rgba8888.assign(static_cast<size_t>(width) * height * 4, 0);
  for (size_t i = 0; i < rgba8888.size(); i += 4)
  {
    rgba8888[i] = kBackgroundR;
    rgba8888[i + 1] = kBackgroundG;
    rgba8888[i + 2] = kBackgroundB;
    rgba8888[i + 3] = kBackgroundA;
  }

  for (auto const & ring : projected)
    StrokeClosedRing(rgba8888, width, height, ring);
  return true;
}

std::string CompletionCardTransientPath()
{
  return GetPlatform().TmpPathForFile(kCompletionCardTransientFile);
}

bool WriteCompletionCardTransient(CompletionCardModel const & model)
{
  std::vector<uint8_t> rgba;
  if (!RasteriseCompletionCard(model, kCompletionCardOutlineSize, kCompletionCardOutlineSize, rgba))
    return false;
  std::string const path = CompletionCardTransientPath();
  int const ok = stbi_write_png(path.c_str(), static_cast<int>(kCompletionCardOutlineSize),
                                static_cast<int>(kCompletionCardOutlineSize), 4, rgba.data(),
                                static_cast<int>(kCompletionCardOutlineSize * 4));
  return ok != 0;
}

void DeleteCompletionCardTransient()
{
  Platform::RemoveFileIfExists(CompletionCardTransientPath());
}

std::string DebugPrint(CompletionCardModel const & model)
{
  size_t vertices = 0;
  for (auto const & ring : model.m_outlineRings)
    vertices += ring.size();
  std::string text = model.m_areaDisplayName + " rings=" + std::to_string(model.m_outlineRings.size()) +
                     " vertices=" + std::to_string(vertices);
  if (model.m_nickname)
    text += " nickname";
  if (model.m_completedDate)
    text += " date";
  return text;
}
}  // namespace street_pixels
