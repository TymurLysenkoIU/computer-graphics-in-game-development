#include "renderer.h"

#include "utils/error_handler.h"

#ifdef RASTERIZATION
#include "renderer/rasterizer/rasterizer_renderer.h"
#endif

#ifdef RAYTRACING
#include "renderer/raytracer/raytracer_renderer.h"
#endif

#ifdef DX12
#include "renderer/dx12/dx12_renderer.h"
#endif


using namespace cg::renderer;

void renderer::set_settings(std::shared_ptr<cg::settings> in_settings)
{
  settings = in_settings;
}

unsigned renderer::get_height()
{
  return settings->height;
}

unsigned renderer::get_width()
{
  return settings->width;
}


std::shared_ptr<renderer> cg::renderer::make_renderer(std::shared_ptr<settings> settings)
{
#ifdef RASTERIZATION
  auto renderer = std::make_shared<rasterization_renderer>();
  renderer->set_settings(settings);
  return renderer;
#endif
#ifdef RAYTRACING
  auto renderer = std::make_shared<cg::renderer::ray_tracing_renderer>();
  renderer->set_settings(settings);
  return renderer;
#endif
#ifdef DX12
  auto renderer = std::make_shared<dx12_renderer>();
  renderer->set_settings(settings);
  return renderer;
#endif

  THROW_ERROR("Type of renderer is not selected");
}

void renderer::move_forward(float delta)
{
    camera->set_position(camera->get_position() + camera->get_direction() * delta);
}

void renderer::move_backward(float delta)
{
    camera->set_position(camera->get_position() - camera->get_direction() * delta);
}

void renderer::move_left(float delta)
{
    camera->set_position(camera->get_position() - camera->get_right() * delta);
}

void renderer::move_right(float delta)
{
    camera->set_position(camera->get_position() + camera->get_right() * delta);
}

void renderer::move_yaw(float delta)
{
  camera->set_theta(delta);
}

void renderer::move_pitch(float delta)
{
  camera->set_phi(delta);
}
