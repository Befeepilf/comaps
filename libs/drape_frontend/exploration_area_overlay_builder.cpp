#include "drape_frontend/exploration_area_overlay_builder.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"

#include "geometry/spline.hpp"

#include "base/string_utils.hpp"

#include <algorithm>

namespace df
{
void ExplorationAreaOverlayBuilder::Build(ref_ptr<dp::GraphicsContext> context,
                                          std::vector<ExplorationAreaOverlayItem> const & items,
                                          ref_ptr<dp::TextureManager> textures,
                                          std::vector<drape_ptr<DrapeApiRenderProperty>> & outlineProperties,
                                          std::vector<drape_ptr<DrapeApiRenderProperty>> & fillProperties)
{
  uint32_t constexpr kMaxSize = 5000;

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

        if (item.m_showCheck && item.m_checkPolyline.size() >= 2)
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

        if (!item.m_name.empty())
        {
          dp::FontDecl font(dp::Color::Black(), 28.0f, dp::Color::White());
          gui::StaticLabel::LabelResult result;
          gui::StaticLabel::CacheStaticText(item.m_name, "\n", dp::Center, font, textures, result);
          glsl::vec2 const pt =
              glsl::ToVec2(MapShape::ConvertToLocal(item.m_labelPoint, property->m_center, kShapeCoordScalar));
          for (gui::StaticLabel::Vertex & v : result.m_buffer)
            v.m_position = glsl::vec3(pt, 0.0f);
          dp::AttributeProvider provider(1, static_cast<uint32_t>(result.m_buffer.size()));
          provider.InitStream(0, gui::StaticLabel::Vertex::GetBindingInfo(), make_ref(result.m_buffer.data()));
          batcher.InsertListOfStrip(context, result.m_state, make_ref(&provider), dp::Batcher::VertexPerQuad);
        }
      }
      if (!property->m_buckets.empty())
        outlineProperties.push_back(std::move(property));
    }

    if (item.m_triangles.size() >= 3 && item.m_fillColor.GetAlpha() > 0)
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

        AreaViewParams avp;
        avp.m_tileCenter = property->m_center;
        avp.m_depthTestEnabled = false;
        avp.m_minVisibleScale = kExplorationAreaOverlayMinZoom;
        avp.m_color = item.m_fillColor;
        BuildingOutline emptyOutline;
        AreaShape(item.m_triangles, std::move(emptyOutline), avp).Draw(context, make_ref(&batcher), textures);
      }
      if (!property->m_buckets.empty())
        fillProperties.push_back(std::move(property));
    }
  }
}
}  // namespace df
