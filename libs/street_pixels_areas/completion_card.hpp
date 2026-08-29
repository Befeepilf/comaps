#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
inline constexpr char const kCompletionCardHeadline[] = "100% explored";
inline constexpr char const kCompletionCardBranding[] = "Street Pixels";
inline constexpr char const kCompletionCardTransientFile[] = "street_pixels_completion_card.png";
inline constexpr char const kCompletionCardShareMime[] = "image/png";
inline constexpr uint32_t kCompletionCardOutlineSize = 512;

struct CompletionCardOptions
{
  std::optional<std::string> nickname;
};

struct CompletionCardSource
{
  std::string m_displayName;
  std::vector<std::vector<m2::PointD>> m_rings;
  std::optional<int64_t> m_completed100At;
  std::string m_competitionLine;
};

struct CompletionCardModel
{
  std::string m_areaDisplayName;
  std::string m_headline;
  std::vector<std::vector<m2::PointD>> m_outlineRings;
  std::optional<std::string> m_nickname;
  std::optional<std::string> m_completedDate;
  std::string m_branding;
  std::string m_competitionLine;
};

struct CompletionCardSharePayload
{
  std::string m_path;
  std::string m_mimeType;
  std::string m_text;
};

std::vector<std::string> CompletionCardPermittedKeys();
std::vector<std::string> CompletionCardDeniedKeys();
std::vector<std::string> PresentFieldNames(CompletionCardModel const & model);
std::string CompletionCardLabelText(CompletionCardModel const & model);

std::optional<CompletionCardModel> ComposeCompletionCard(CompletionCardSource const & source,
                                                         CompletionCardOptions const & options = {});

std::vector<std::vector<m2::PointD>> ProjectOutlineToPixels(std::vector<std::vector<m2::PointD>> const & rings,
                                                            uint32_t width, uint32_t height,
                                                            double padFraction = 0.08);

bool RasteriseCompletionCard(CompletionCardModel const & model, uint32_t width, uint32_t height,
                             std::vector<uint8_t> & rgba8888);
std::string CompletionCardTransientPath();
bool WriteCompletionCardTransient(CompletionCardModel const & model);
void DeleteCompletionCardTransient();

std::string DebugPrint(CompletionCardModel const & model);
}  // namespace street_pixels
