#pragma once

#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/exploration_area_overlay.hpp"

#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include <vector>

namespace dp
{
class GraphicsContext;
}  // namespace dp

namespace df
{
class ExplorationAreaOverlayBuilder
{
public:
  void Build(ref_ptr<dp::GraphicsContext> context, std::vector<ExplorationAreaOverlayItem> const & items,
             ref_ptr<dp::TextureManager> textures,
             std::vector<drape_ptr<DrapeApiRenderProperty>> & outlineProperties,
             std::vector<drape_ptr<DrapeApiRenderProperty>> & fillProperties,
             std::vector<drape_ptr<DrapeApiRenderProperty>> & chromeProperties);
};
}  // namespace df
