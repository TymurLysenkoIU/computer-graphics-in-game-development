#include "renderer/rasterizer/rasterizer.h"
#include "renderer/renderer.h"
#include "resource.h"


namespace cg::renderer
{
class rasterization_renderer : public renderer
{
public:
  void init() override;
  void destroy() override;

  void update() override;
  void render() override;

protected:
  std::shared_ptr<resource<unsigned_color>> render_target;
  std::shared_ptr<resource<float>> depth_buffer;

  std::shared_ptr<rasterizer<vertex, unsigned_color>> rasterizer;
};
} // namespace cg::renderer
