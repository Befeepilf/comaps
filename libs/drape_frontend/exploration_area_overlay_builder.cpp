#include "drape_frontend/exploration_area_overlay_builder.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/color.hpp"
#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "shaders/programs.hpp"

#include "geometry/spline.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace df
{
namespace
{
float constexpr kRingWidthPx = 3.5f;
int constexpr kRingSegments = 24;

void AppendAnnulus(std::vector<gpu::SolidTexturingVertex> & buffer, glsl::vec4 const & position,
                    glsl::vec2 const & extraOffset, float innerR, float outerR, float startAngle, float endAngle,
                    gpu::SolidTexturingVertex::TTexCoord const & uv)
{
  using V = gpu::SolidTexturingVertex;
  for (int i = 0; i < kRingSegments; ++i)
  {
    float const t0 = static_cast<float>(i) / static_cast<float>(kRingSegments);
    float const t1 = static_cast<float>(i + 1) / static_cast<float>(kRingSegments);
    float const a0 = startAngle + (endAngle - startAngle) * t0;
    float const a1 = startAngle + (endAngle - startAngle) * t1;
    float const c0 = std::cos(a0);
    float const s0 = std::sin(a0);
    float const c1 = std::cos(a1);
    float const s1 = std::sin(a1);
    auto vtx = [&](float r, float c, float s)
    {
      return V(position, glsl::vec2(extraOffset.x + r * s, extraOffset.y - r * c), uv);
    };
    buffer.push_back(vtx(innerR, c0, s0));
    buffer.push_back(vtx(outerR, c0, s0));
    buffer.push_back(vtx(outerR, c1, s1));
    buffer.push_back(vtx(innerR, c0, s0));
    buffer.push_back(vtx(outerR, c1, s1));
    buffer.push_back(vtx(innerR, c1, s1));
  }
}

void DrawCachedText(ref_ptr<dp::GraphicsContext> context, dp::Batcher & batcher, glsl::vec2 const & pt,
                     glsl::vec2 const & extraOffset, gui::StaticLabel::LabelResult & result)
{
  for (gui::StaticLabel::Vertex & v : result.m_buffer)
  {
    v.m_position = glsl::vec3(pt, 0.0f);
    v.m_normal = v.m_normal + extraOffset;
  }
  dp::AttributeProvider provider(1, static_cast<uint32_t>(result.m_buffer.size()));
  provider.InitStream(0, gui::StaticLabel::Vertex::GetBindingInfo(), make_ref(result.m_buffer.data()));
  batcher.InsertListOfStrip(context, result.m_state, make_ref(&provider), dp::Batcher::VertexPerQuad);
}
}  // namespace

void ExplorationAreaOverlayBuilder::Build(ref_ptr<dp::GraphicsContext> context,
                                           std::vector<ExplorationAreaOverlayItem> const & items,
                                           ref_ptr<dp::TextureManager> textures,
                                           std::vector<drape_ptr<DrapeApiRenderProperty>> & outlineProperties,
                                           std::vector<drape_ptr<DrapeApiRenderProperty>> & fillProperties,
                                           std::vector<drape_ptr<DrapeApiRenderProperty>> & chromeProperties)
{
  uint32_t constexpr kMaxSize = 5000;
  base::Timer buildTimer;
  LOG(LINFO, ("StreetPixels overlay gpu build start", "items", items.size()));

  for (auto const & item : items)
  {
    std::string const id = "sp_area_" + strings::to_string(item.m_compactIndex);

    if (!item.m_rings.empty())
    {
      dp::Batcher batcher(kMaxSize, kMaxSize);
      batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
      auto property = make_unique_dp<DrapeApiRenderProperty>();
      property->m_center = item.m_bounds.Center();
      property->m_id = id;
      {
        dp::SessionGuard guard(context, batcher,
                               [&property](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
        { property->m_buckets.emplace_back(state, std::move(b)); });

        for (auto const & ring : item.m_rings)
        {
          if (ring.size() < 2)
            continue;
          m2::SharedSpline spline(ring);
          LineViewParams lvp;
          lvp.m_tileCenter = property->m_center;
          lvp.m_depthTestEnabled = false;
          lvp.m_minVisibleScale = kExplorationAreaOverlayMinZoom;
          lvp.m_cap = dp::RoundCap;
          lvp.m_join = dp::RoundJoin;
          lvp.m_color = item.m_outlineColor;
          lvp.m_width = item.m_outlineWidthPx;
          LineShape(spline, lvp).Draw(context, make_ref(&batcher), textures);
        }
      }
      if (!property->m_buckets.empty())
        outlineProperties.push_back(std::move(property));
    }

    bool const drawFill = item.m_triangles.size() >= 3 && item.m_fillColor.GetAlpha() > 0;
    bool const drawCheck = item.m_showCheck && item.m_checkPolyline.size() >= 2;
    if (drawFill || drawCheck)
    {
      dp::Batcher batcher(kMaxSize, kMaxSize);
      batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
      auto property = make_unique_dp<DrapeApiRenderProperty>();
      property->m_center = item.m_bounds.Center();
      property->m_id = id;
      {
        dp::SessionGuard guard(context, batcher,
                               [&property](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
        { property->m_buckets.emplace_back(state, std::move(b)); });

        if (drawFill)
        {
          AreaViewParams avp;
          avp.m_tileCenter = property->m_center;
          avp.m_depthTestEnabled = false;
          avp.m_minVisibleScale = kExplorationAreaOverlayMinZoom;
          avp.m_color = item.m_fillColor;
          BuildingOutline emptyOutline;
          AreaShape(item.m_triangles, std::move(emptyOutline), avp).Draw(context, make_ref(&batcher), textures);
        }
        if (drawCheck)
        {
          m2::SharedSpline checkSpline(item.m_checkPolyline);
          LineViewParams cvp;
          cvp.m_tileCenter = property->m_center;
          cvp.m_depthTestEnabled = false;
          cvp.m_minVisibleScale = kExplorationAreaOverlayMinZoom;
          cvp.m_cap = dp::RoundCap;
          cvp.m_join = dp::RoundJoin;
          cvp.m_color = item.m_outlineColor;
          cvp.m_width = std::max(item.m_outlineWidthPx, 5.0f);
          LineShape(checkSpline, cvp).Draw(context, make_ref(&batcher), textures);
        }
      }
      if (!property->m_buckets.empty())
        fillProperties.push_back(std::move(property));
    }

    if (item.m_showName || item.m_showPct)
    {
      dp::Batcher batcher(kMaxSize, kMaxSize);
      batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
      auto property = make_unique_dp<DrapeApiRenderProperty>();
      property->m_center = item.m_bounds.Center();
      property->m_id = id + "_chrome";
      {
        dp::SessionGuard guard(context, batcher,
                               [&property](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
        { property->m_buckets.emplace_back(state, std::move(b)); });

        glsl::vec2 const pt =
            glsl::ToVec2(MapShape::ConvertToLocal(item.m_labelPoint, property->m_center, kShapeCoordScalar));

        if (item.m_showName && !item.m_name.empty())
        {
          dp::FontDecl font(dp::Color::Black(), item.m_fontSize, dp::Color::White());
          gui::StaticLabel::LabelResult result;
          gui::StaticLabel::CacheStaticText(item.m_name, "\n", dp::Center, font, textures, result);
          DrawCachedText(context, batcher, pt, glsl::vec2(0.0f, 0.0f), result);
        }

        if (item.m_showPct)
        {
          float const vs = static_cast<float>(VisualParams::Instance().GetVisualScale());
          glsl::vec2 const ringOff(item.m_ringOffsetPx.x * vs, item.m_ringOffsetPx.y * vs);
          glsl::vec4 const position(pt.x, pt.y, 0.0f, 0.0f);

          dp::TextureManager::ColorRegion trackRegion;
          textures->GetColorRegion(dp::Color(160, 160, 160, 220), trackRegion);
          dp::TextureManager::ColorRegion arcRegion;
          textures->GetColorRegion(item.m_ringColor, arcRegion);

          using V = gpu::SolidTexturingVertex;
          std::vector<V> ringVerts;
          ringVerts.reserve(static_cast<size_t>(kRingSegments) * 12);
          float const outerR = kExplorationAreaOverlayRingRadiusPx * vs;
          float const innerR = std::max(0.0f, outerR - kRingWidthPx * vs);
          m2::PointF const trackCenter = trackRegion.GetTexRect().Center();
          m2::PointF const arcCenter = arcRegion.GetTexRect().Center();
          V::TTexCoord const trackUv(trackCenter.x, trackCenter.y);
          V::TTexCoord const arcUv(arcCenter.x, arcCenter.y);
          float const twoPi = static_cast<float>(2.0 * math::pi);
          AppendAnnulus(ringVerts, position, ringOff, innerR, outerR, 0.0f, twoPi, trackUv);
          float const frac = static_cast<float>(std::clamp(item.m_fraction, 0.0, 1.0));
          if (frac > 0.0f)
            AppendAnnulus(ringVerts, position, ringOff, innerR, outerR, 0.0f, twoPi * frac, arcUv);

          auto state = CreateRenderState(gpu::Program::Texturing, DepthLayer::GuiLayer);
          state.SetProgram3d(gpu::Program::TexturingBillboard);
          state.SetDepthTestEnabled(false);
          state.SetColorTexture(trackRegion.GetTexture());
          dp::AttributeProvider ringProvider(1, static_cast<uint32_t>(ringVerts.size()));
          ringProvider.InitStream(0, V::GetBindingInfo(), make_ref(ringVerts.data()));
          batcher.InsertTriangleList(context, state, make_ref(&ringProvider));

          if (!item.m_percentText.empty())
          {
            float const pctSize = std::max(12.0f, item.m_fontSize * 0.55f);
            dp::FontDecl font(dp::Color::Black(), pctSize, dp::Color::White());
            gui::StaticLabel::LabelResult result;
            gui::StaticLabel::CacheStaticText(item.m_percentText, "\n", dp::Center, font, textures, result);
            DrawCachedText(context, batcher, pt, ringOff, result);
          }
        }
      }
      if (!property->m_buckets.empty())
        chromeProperties.push_back(std::move(property));
    }
  }
  LOG(LINFO, ("StreetPixels overlay gpu build ms", buildTimer.ElapsedMilliseconds(), "items", items.size(),
              "outlines", outlineProperties.size(), "fills", fillProperties.size(), "chrome",
              chromeProperties.size()));
}
}  // namespace df
