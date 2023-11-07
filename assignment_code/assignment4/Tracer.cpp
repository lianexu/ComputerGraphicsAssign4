#include "Tracer.hpp"

#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <algorithm>

#include "gloo/Transform.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/lights/AmbientLight.hpp"

#include "gloo/Image.hpp"
#include "Illuminator.hpp"

namespace GLOO {
void Tracer::Render(const Scene& scene, const std::string& output_file) {
  scene_ptr_ = &scene;

  auto& root = scene_ptr_->GetRootNode();
  tracing_components_ = root.GetComponentPtrsInChildren<TracingComponent>();
  light_components_ = root.GetComponentPtrsInChildren<LightComponent>();


  Image image(image_size_.x, image_size_.y);


  for (size_t y = 0; y < image_size_.y; y++) {
    for (size_t x = 0; x < image_size_.x; x++) {
      // TODO: For each pixel, cast a ray, and update its value in the image.
      float x_norm = float(x) * (2.0/image_size_.x) - 1;
      float y_norm = float(y) * (2.0/image_size_.y) - 1;
      Ray ray = camera_.GenerateRay(glm::vec2(x_norm,y_norm));
      
      HitRecord record;
      glm::vec3 color = TraceRay(ray, max_bounces_, record);
      image.SetPixel(x,y,color);
      // std::cout << "x" << x_norm << std::endl;
      // std::cout << "y" << y_norm << std::endl;
      
    }
  }

  if (output_file.size())
    image.SavePNG(output_file);
}


glm::vec3 Tracer::TraceRay(const Ray& ray,
                           size_t bounces,
                           HitRecord& record) const {
  // TODO: Compute the color for the cast ray.
    glm::vec3 pixel_color(1.0f);
    float t_min = camera_.GetTMin();
    for (const auto& component : tracing_components_) {
      glm::mat4 component_to_world = component->GetNodePtr()->GetTransform().GetLocalToWorldMatrix();
      glm::mat4 world_to_component = glm::inverse(component_to_world);
      Ray ray_temp = ray;
      ray_temp.ApplyTransform(world_to_component);

      const auto& hittable = component->GetHittable();
      if(hittable.Intersect(ray_temp, t_min, record)){
        record.normal = glm::normalize(record.normal * glm::transpose(glm::mat3(component_to_world)));
        pixel_color = component->GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial().GetAmbientColor();
      }


    }

    if (record.time < std::numeric_limits<float>::max()){
      return pixel_color;
    }else{
      return GetBackgroundColor(ray.GetDirection());
    }
 }


glm::vec3 Tracer::GetBackgroundColor(const glm::vec3& direction) const {
  if (cube_map_ != nullptr) {
    return cube_map_->GetTexel(direction);
  } else
    return background_color_;
}
}  // namespace GLOO
