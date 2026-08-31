#include "drape_frontend/exploration_area_overlay_renderer.hpp"

#include "drape_frontend/exploration_area_overlay.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/glsl_types.hpp"
#include "drape/gpu_program.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "shaders/program_params.hpp"
#include "shaders/programs.hpp"

#include "base/matrix.hpp"

namespace df
{
void ExplorationAreaOverlayRenderer::BuildProperties(
    ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
    std::vector<drape_ptr<DrapeApiRenderProperty>> const & properties)
{
  for (auto const & property : properties)
  {
    for (auto const & bucket : property->m_buckets)
    {
      auto program = mng->GetProgram(bucket.first.GetProgram<gpu::Program>());
      program->Bind();
      bucket.second->GetBuffer()->Build(context, program);
    }
  }
}

void ExplorationAreaOverlayRenderer::SetProperties(ref_ptr<dp::GraphicsContext> context,
                                                    ref_ptr<gpu::ProgramManager> mng,
                                                    std::vector<drape_ptr<DrapeApiRenderProperty>> && outlines,
                                                    std::vector<drape_ptr<DrapeApiRenderProperty>> && fills,
                                                    std::vector<drape_ptr<DrapeApiRenderProperty>> && chrome)
{
  m_outlineProperties = std::move(outlines);
  m_fillProperties = std::move(fills);
  m_chromeProperties = std::move(chrome);
  BuildProperties(context, mng, m_outlineProperties);
  BuildProperties(context, mng, m_fillProperties);
  BuildProperties(context, mng, m_chromeProperties);
}

void ExplorationAreaOverlayRenderer::Clear()
{
  m_outlineProperties.clear();
  m_fillProperties.clear();
  m_chromeProperties.clear();
}

void ExplorationAreaOverlayRenderer::RenderProperties(ref_ptr<dp::GraphicsContext> context,
                                                      ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                                                      FrameValues const & frameValues,
                                                      std::vector<drape_ptr<DrapeApiRenderProperty>> const & properties,
                                                      float opacity)
{
  for (auto const & property : properties)
  {
    math::Matrix<float, 4, 4> const mv = screen.GetModelView(property->m_center, kShapeCoordScalar);
    for (auto const & bucket : property->m_buckets)
    {
      auto const p2d = bucket.first.GetProgram<gpu::Program>();
      auto const programId =
          screen.isPerspective() ? bucket.first.GetProgram3d<gpu::Program>() : p2d;
      auto program = mng->GetProgram(programId);
      program->Bind();
      bucket.second->GetBuffer()->Build(context, program);
      dp::ApplyState(context, program, bucket.first);

      if (p2d == gpu::Program::TextOutlinedGui || p2d == gpu::Program::TextStaticOutlinedGui)
      {
        gpu::GuiProgramParams params;
        frameValues.SetTo(params);
        params.m_modelView = glsl::make_mat4(mv.m_data);
        auto const & glyphParams = VisualParams::Instance().GetGlyphVisualParams();
        params.m_contrastGamma = glsl::vec2(glyphParams.m_guiContrast, glyphParams.m_guiGamma);
        params.m_isOutlinePass = 0.0f;
        params.m_opacity = opacity;
        mng->GetParamsSetter()->Apply(context, program, params);
      }
      else
      {
        gpu::MapProgramParams params;
        frameValues.SetTo(params);
        params.m_modelView = glsl::make_mat4(mv.m_data);
        params.m_opacity = opacity;
        mng->GetParamsSetter()->Apply(context, program, params);
      }

      bucket.second->Render(context, bucket.first.GetDrawAsLine());
    }
  }
}

void ExplorationAreaOverlayRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                             ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (!m_enabled)
    return;

  float const fillOpacity = OverlayFillOpacityFactor(GetZoomLevel(screen.GetScale()), m_zoomRange.m_fillMinZoom,
                                                      m_zoomRange.m_fillMaxZoom);
  if (fillOpacity > 0.0f)
    RenderProperties(context, mng, screen, frameValues, m_fillProperties, fillOpacity);

  if (zoomLevel >= kExplorationAreaOverlayMinZoom)
    RenderProperties(context, mng, screen, frameValues, m_outlineProperties, 1.0f);

  if (OverlayChromeVisible(zoomLevel, m_zoomRange.m_labelMinZoom, m_zoomRange.m_labelMaxZoom))
    RenderProperties(context, mng, screen, frameValues, m_chromeProperties, 1.0f);
}
}  // namespace df
