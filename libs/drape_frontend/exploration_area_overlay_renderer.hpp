#pragma once

#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/exploration_area_overlay.hpp"
#include "drape_frontend/frame_values.hpp"

#include "shaders/program_manager.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include <vector>

namespace df
{
class ExplorationAreaOverlayRenderer final
{
public:
  void SetEnabled(bool enabled) { m_enabled = enabled; }
  bool IsEnabled() const { return m_enabled; }

  void SetProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     std::vector<drape_ptr<DrapeApiRenderProperty>> && outlines,
                     std::vector<drape_ptr<DrapeApiRenderProperty>> && fills,
                     std::vector<drape_ptr<DrapeApiRenderProperty>> && chrome);
  void Clear();

  void SetZoomRange(ExplorationAreaOverlayZoomRange const & range) { m_zoomRange = range; }

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, FrameValues const & frameValues);

private:
  void BuildProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                       std::vector<drape_ptr<DrapeApiRenderProperty>> const & properties);
  void RenderProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                        ScreenBase const & screen, FrameValues const & frameValues,
                        std::vector<drape_ptr<DrapeApiRenderProperty>> const & properties, float opacity);

  bool m_enabled = false;
  ExplorationAreaOverlayZoomRange m_zoomRange;
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_outlineProperties;
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_fillProperties;
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_chromeProperties;
};
}  // namespace df
