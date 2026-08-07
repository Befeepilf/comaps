#include "drape_frontend/exploration_area_overlay_builder.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/batcher.hpp"

#include "geometry/spline.hpp"

#include "base/string_utils.hpp"

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
