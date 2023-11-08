#include "Tracer.hpp"

#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <algorithm>

#include "gloo/Transform.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/lights/AmbientLight.hpp"

#include "gloo/Image.hpp"
#include "Illuminator.hpp"


#include "glm/ext.hpp" //for printing
#include "glm/gtx/string_cast.hpp"

namespace GLOO {
void Tracer::Render(const Scene& scene, const std::string& output_file) {
  scene_ptr_ = &scene;

  auto& root = scene_ptr_->GetRootNode();
  tracing_components_ = root.GetComponentPtrsInChildren<TracingComponent>();
  light_components_ = root.GetComponentPtrsInChildren<LightComponent>();


  Image image(image_size_.x, image_size_.y);
  // glm::vec3 scene_ambient = root.GetComponentPtr<MaterialComponent>()->GetMaterial().GetAmbientColor();
    
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
      // root->GetComponentPtr<MaterialComponent>()->GetMaterial().GetAmbientColor();
    }
  }

  if (output_file.size())
    image.SavePNG(output_file);
}


glm::vec3 Tracer::TraceRay(const Ray& ray,
                           size_t bounces,
                           HitRecord& record) const {
  // TODO: Compute the color for the cast ray.
    glm::vec3 pixel_color(0.0f);
    float t_min = camera_.GetTMin();
    for (const auto& component : tracing_components_) {
      glm::mat4 component_to_world = component->GetNodePtr()->GetTransform().GetLocalToWorldMatrix();
      glm::mat4 world_to_component = glm::inverse(component_to_world);
      Ray ray_temp = ray;
      ray_temp.ApplyTransform(world_to_component);

      const auto& hittable = component->GetHittable();
      
      if(hittable.Intersect(ray_temp, t_min, record)){
        pixel_color = glm::vec3(0.0f);
        record.normal = glm::normalize(record.normal * glm::transpose(glm::mat3(component_to_world)));
        glm::vec3 hit_pos = ray.At(record.time);

        glm::vec3 k_diffuse = component->GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial().GetDiffuseColor();
        glm::vec3 k_specular = component->GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial().GetSpecularColor();
        glm::vec3 k_ambient = component->GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial().GetAmbientColor();
        float shininess = component->GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial().GetShininess();

        for (auto& light : light_components_){
          if (light->GetLightPtr()->GetType() == LightType::Point || light->GetLightPtr()->GetType() == LightType::Directional){
            glm::vec3 dir_to_light(0.0f, 0.0f, 0.0f);
            glm::vec3 intensity(0.0f, 0.0f, 0.0f);
            float dist_to_light = 0.0f;
            Illuminator::GetIllumination(*light, hit_pos, dir_to_light, intensity, dist_to_light);

            // Diffuse Shading
            glm::vec3 I_Diffuse = GetIDiffuse(k_diffuse, dir_to_light, intensity, record.normal);

            //Specular Shading
            glm::vec3 surface_to_eye = -ray.GetDirection();
            glm::vec3 I_Specular = GetISpecular(shininess, k_specular, surface_to_eye, dir_to_light, intensity, record.normal);

            pixel_color += I_Diffuse + I_Specular;
          }else{ // Ambient Lighting
            // Ambient Shading
            glm::vec3 L_Ambient = light->GetLightPtr()->GetDiffuseColor();
            glm::vec3 I_Ambient = L_Ambient * k_ambient;
            pixel_color += I_Ambient;
          }
          }
      }
    }
      

    if (record.time < std::numeric_limits<float>::max()){
      return pixel_color;
    }else{
      return GetBackgroundColor(ray.GetDirection());
    }
 }

 glm::vec3 Tracer::GetIDiffuse(glm::vec3 k_diffuse, glm::vec3 dir_to_light, glm::vec3 intensity, glm::vec3 normal) const{
  float clamped = std::max(0.0f, glm::dot(dir_to_light, normal));
  glm::vec3 I_Diffuse = clamped * intensity * k_diffuse;
  return I_Diffuse;
 }

 glm::vec3 Tracer::GetISpecular(float shininess, glm::vec3 k_specular, glm::vec3 surface_to_eye, glm::vec3 dir_to_light, glm::vec3 intensity, glm::vec3 normal) const{
  auto R = -dir_to_light + 2 * glm::dot(dir_to_light, normal) * normal;
  float clamped = std::max(0.0f, glm::dot(surface_to_eye, R));
  glm::vec3 I_Specular = pow(clamped, shininess) * intensity * k_specular;
  return I_Specular;
 }


glm::vec3 Tracer::GetBackgroundColor(const glm::vec3& direction) const {
  if (cube_map_ != nullptr) {
    return cube_map_->GetTexel(direction);
  } else
    return background_color_;
}

}  // namespace GLOO
