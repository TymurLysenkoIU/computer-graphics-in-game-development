#include "renderer/raytracer/raytracer.h"
#include "renderer/renderer.h"
#include "resource.h"


namespace cg::renderer
{
class ray_tracing_renderer : public renderer
{
public:
  void init() override;
  void destroy() override;

  void update() override;
  void render() override;

protected:
  std::shared_ptr<resource<unsigned_color>> render_target;

  std::shared_ptr<raytracer<vertex, unsigned_color>> raytracer;
  std::shared_ptr<raytracer < vertex, unsigned_color>
  >
  shadow_raytracer;

  std::vector<light> lights;
};
} // namespace cg::renderer
