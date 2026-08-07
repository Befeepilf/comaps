#pragma once

#include "drape_frontend/drape_api_builder.hpp"
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

  void SetOutlineProperties(std::vector<drape_ptr<DrapeApiRenderProperty>> && properties);
  void SetFillProperties(std::vector<drape_ptr<DrapeApiRenderProperty>> && properties);
  void Clear();

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, FrameValues const & frameValues);

private:
  void RenderProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                        ScreenBase const & screen, FrameValues const & frameValues,
                        std::vector<drape_ptr<DrapeApiRenderProperty>> const & properties);

  bool m_enabled = true;
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_outlineProperties;
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_fillProperties;
};
}  // namespace df
