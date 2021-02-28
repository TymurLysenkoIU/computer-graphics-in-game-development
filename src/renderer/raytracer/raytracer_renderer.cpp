#include "raytracer_renderer.h"

#include "utils/resource_utils.h"


void cg::renderer::ray_tracing_renderer::init()
{
  render_target =
    std::make_shared<resource<unsigned_color>>(
      settings->width,
      settings->height
    );

  model = std::make_shared<world::model>();
  model->load_obj(settings->model_path);

  camera = std::make_shared<world::camera>();
  camera->set_width(static_cast<float>(settings->width));
  camera->set_height(static_cast<float>(settings->height));
  camera->set_position(float3{ settings->camera_position[0],
                               settings->camera_position[1],
                               settings->camera_position[2] });
  camera->set_theta(settings->camera_theta);
  camera->set_phi(settings->camera_phi);
  camera->set_angle_of_view(settings->camera_angle_of_view);
  camera->set_z_near(settings->camera_z_near);
  camera->set_z_far(settings->camera_z_far);

  raytracer = std::make_shared<cg::renderer::raytracer<vertex, unsigned_color>>();
  raytracer->set_render_target(render_target);
  raytracer->set_viewport(settings->width, settings->height);
  raytracer->set_per_shape_vertex_buffer(model->get_per_shape_buffer());

  shadow_raytracer =
    std::make_shared<cg::renderer::raytracer<vertex, unsigned_color>>();
  shadow_raytracer->SSAA_factor = 1;
  shadow_raytracer->max_depth = 1;

  for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
      lights.push_back({
        float3{ 0 + x * 0.05f, 1.58f, -0.03f + y * 0.05f },
        float3{ 121.f, 58.f, 122.f } / 255.f / 9.f
      });
    }
}

void cg::renderer::ray_tracing_renderer::destroy()
{
}

void cg::renderer::ray_tracing_renderer::update()
{
}

void cg::renderer::ray_tracing_renderer::render()
{
  raytracer->clear_render_target({ 0, 0, 0 });

  raytracer->miss_shader = [](const ray& ray) {
    payload payload = {};

    payload.color = {
      10.f / 255.f * (0.8f - ray.direction.y * 0.8f),
      12.f / 255.f * (0.8f - ray.direction.y * 0.8f),
      23.f / 255.f
    };

    return payload;
  };

  raytracer->closest_hit_shader = [&](
    const ray& ray,
    payload& payload,
    const triangle<cg::vertex>& triangle
  ) {
    float3 result_color = triangle.emissive;
    float3 position = ray.position + ray.direction * payload.t;
    float3 normal = payload.bary.x * triangle.na + payload.bary.y * triangle.nb + payload.bary.z * triangle.nc;

    for (auto& light : lights)
    {
      cg::renderer::ray to_light(position, light.position - position);

      auto shadow_payload = shadow_raytracer->trace_ray(
        to_light,
        1,
        length(light.position - position)
      );

      float shadow_factor = 0.4f;
      if (shadow_payload.t == -1.f)
        shadow_factor = 1.f;

      result_color +=
        triangle.diffuse * light.color *
        std::max(0.f, dot(normal, to_light.direction)) *
        shadow_factor;
    }

    if (payload.depth > 0)
    {
      float3 ref_direction = ray.direction - 2 * dot(ray.direction, normal) * normal;
      float3 ref_jitter = float3(
        raytracer->get_random(omp_get_thread_num() + clock()),
        raytracer->get_random(omp_get_thread_num() + clock()),
        raytracer->get_random(omp_get_thread_num() + clock())
      ) / 3.f;

      ref_direction = normalize(ref_direction + ref_jitter);

      cg::renderer::ray ref_ray(position, ref_direction);
      auto ref_result = raytracer->trace_ray(ref_ray, payload.depth - 1);
      result_color += 0.3f * float3(
        ref_result.color.r,
        ref_result.color.g,
        ref_result.color.b
      );
    }

    payload.color = cg::color::from_float3(result_color);
    return payload;
  };

  raytracer->build_acceleration_structure();

  shadow_raytracer->acceleration_structures = raytracer->acceleration_structures;
  shadow_raytracer->miss_shader = [](const ray& ray) {
    payload payload = {};
    payload.t = -1.f;
    return payload;
  };
  shadow_raytracer->closest_hit_shader = [](
    const ray& ray,
    payload& payload,
    const triangle<vertex>& triangle
  ) {
      return payload;
  };

  raytracer->ray_generation(
    camera->get_position(),
    camera->get_direction(),
    camera->get_right(),
    camera->get_up()
  );

  cg::utils::save_resource(*render_target, settings->result_path);
}
