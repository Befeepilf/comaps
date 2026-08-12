#include "drape_frontend/exploration_area_overlay_renderer.hpp"

#include "drape_frontend/exploration_area_overlay.hpp"
#include "drape_frontend/shape_view_params.hpp"

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
                                                   std::vector<drape_ptr<DrapeApiRenderProperty>> && fills)
{
  m_outlineProperties = std::move(outlines);
  m_fillProperties = std::move(fills);
  BuildProperties(context, mng, m_outlineProperties);
  BuildProperties(context, mng, m_fillProperties);
}

void ExplorationAreaOverlayRenderer::Clear()
{
  m_outlineProperties.clear();
  m_fillProperties.clear();
}

void ExplorationAreaOverlayRenderer::RenderProperties(ref_ptr<dp::GraphicsContext> context,
                                                      ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                                                      FrameValues const & frameValues,
                                                      std::vector<drape_ptr<DrapeApiRenderProperty>> const & properties)
{
  for (auto const & property : properties)
  {
    math::Matrix<float, 4, 4> const mv = screen.GetModelView(property->m_center, kShapeCoordScalar);
    for (auto const & bucket : property->m_buckets)
    {
      auto program = mng->GetProgram(bucket.first.GetProgram<gpu::Program>());
      program->Bind();
      dp::ApplyState(context, program, bucket.first);

      gpu::MapProgramParams params;
      frameValues.SetTo(params);
      params.m_modelView = glsl::make_mat4(mv.m_data);
      mng->GetParamsSetter()->Apply(context, program, params);

      bucket.second->Render(context, bucket.first.GetDrawAsLine());
    }
  }
}

void ExplorationAreaOverlayRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                            ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (!m_enabled)
    return;
  if (zoomLevel < kExplorationAreaOverlayMinZoom)
    return;

  bool const showFill = zoomLevel <= kExplorationAreaOverlayNeighbourhoodMaxZoom;
  if (showFill)
    RenderProperties(context, mng, screen, frameValues, m_fillProperties);

  RenderProperties(context, mng, screen, frameValues, m_outlineProperties);
}
}  // namespace df
