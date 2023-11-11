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

namespace GLOO
{
  void Tracer::Render(const Scene &scene, const std::string &output_file)
  {
    scene_ptr_ = &scene;

    auto &root = scene_ptr_->GetRootNode();
    tracing_components_ = root.GetComponentPtrsInChildren<TracingComponent>();
    light_components_ = root.GetComponentPtrsInChildren<LightComponent>();

    Image image(image_size_.x, image_size_.y);

    for (size_t y = 0; y < image_size_.y; y++)
    {
      for (size_t x = 0; x < image_size_.x; x++)
      {
        // TODO: For each pixel, cast a ray, and update its value in the image.
        float x_norm = float(x) * (2.0 / image_size_.x) - 1;
        float y_norm = float(y) * (2.0 / image_size_.y) - 1;
        Ray ray = camera_.GenerateRay(glm::vec2(x_norm, y_norm));

        HitRecord record;
        glm::vec3 color = TraceRay(ray, max_bounces_, record);
        image.SetPixel(x, y, color);
        // std::cout << "x" << x_norm << std::endl;
        // std::cout << "y" << y_norm << std::endl;
        // root->GetComponentPtr<MaterialComponent>()->GetMaterial().GetAmbientColor();
      }
    }

    if (output_file.size())
      image.SavePNG(output_file);
  }


  bool Tracer::IntersectInScene(const Ray &ray,
                             HitRecord& record,
                             Material &material) const{
    float t_min = camera_.GetTMin();
    bool res = false;
    for (const auto &component : tracing_components_)
    {
      glm::mat4 component_to_world = component->GetNodePtr()->GetTransform().GetLocalToWorldMatrix();
      glm::mat4 world_to_component = glm::inverse(component_to_world);
      Ray ray_temp = ray;
      ray_temp.ApplyTransform(world_to_component);

      const auto &hittable = component->GetHittable();
      bool hit = hittable.Intersect(ray_temp, t_min, record);
      if (hit && record.time < std::numeric_limits<float>::max()){
          res = true;
          // record.normal = glm::normalize(record.normal * glm::transpose(glm::mat3(world_to_component)));
          record.normal = glm::normalize(glm::mat3(glm::transpose(world_to_component)) * record.normal);
          material = component->GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial();
      }
    }
    return res;
    }

  glm::vec3 Tracer::TraceRay(const Ray &ray,
                             size_t bounces,
                             HitRecord &record) const
  {
    // TODO: Compute the color for the cast ray.
    glm::vec3 pixel_color(0.0f);
    glm::vec3 I_Direct(0.0f);
    glm::vec3 I_Indirect(0.0f);

    // float t_min = camera_.GetTMin();
    Material material;
    bool intersect = IntersectInScene(ray, record, material);

    if(intersect)
    {
      I_Direct = glm::vec3(0.0f);
      glm::vec3 hit_pos = ray.At(record.time);

      glm::vec3 k_diffuse = material.GetDiffuseColor();
      glm::vec3 k_specular = material.GetSpecularColor();
      glm::vec3 k_ambient = material.GetAmbientColor();
      float shininess = material.GetShininess();

      for (auto &light : light_components_)
      {
        if (light->GetLightPtr()->GetType() == LightType::Point || light->GetLightPtr()->GetType() == LightType::Directional)
        {
          glm::vec3 dir_to_light(0.0f, 0.0f, 0.0f);
          glm::vec3 intensity(0.0f, 0.0f, 0.0f);
          float dist_to_light = 0.0f;
          Illuminator::GetIllumination(*light, hit_pos, dir_to_light, intensity, dist_to_light);

          // Shadow check
          glm::vec3 R_shadow_small = dir_to_light;
          float epsilon = 0.01;
          R_shadow_small.x *= epsilon;
          R_shadow_small.y *= epsilon;
          R_shadow_small.z *= epsilon;
          Ray shadow_ray(hit_pos + R_shadow_small, dir_to_light); // Shadow ray is in direction of light, moved over by epsilon
          HitRecord record2; // trashed variable for shadows
          Material material2; // trashed variable for shadows 
          bool shadowed = IntersectInScene(shadow_ray, record2, material2);
          if (shadowed == false || record2.time > dist_to_light || shadows_enabled_ == false) {
            // Diffuse Shading
            glm::vec3 I_Diffuse = GetIDiffuse(k_diffuse, dir_to_light, intensity, record.normal);

            // Specular Shading
            glm::vec3 surface_to_eye = -ray.GetDirection();
            glm::vec3 I_Specular = GetISpecular(shininess, k_specular, surface_to_eye, dir_to_light, intensity, record.normal);

            I_Direct += I_Diffuse + I_Specular;
          }

        } else { // Ambient Lighting, Ambient Shading (These are not affected by showdows)
          glm::vec3 L_Ambient = light->GetLightPtr()->GetDiffuseColor();
          glm::vec3 I_Ambient = L_Ambient * k_ambient;
          I_Direct += I_Ambient;
        }
      }

      if (bounces > 0 && glm::length(k_specular) > 1e-2f)
      {
        HitRecord recursive_record;
        // glm::vec3 surface_to_eye = -ray.GetDirection();

        glm::vec3 R = glm::reflect(ray.GetDirection(), record.normal);
        glm::vec3 R_small = R;
        float epsilon = 0.01;
        R_small.x *= epsilon;
        R_small.y *= epsilon;
        R_small.z *= epsilon;
        Ray recursive_ray(hit_pos + R_small, R);
        I_Indirect += TraceRay(recursive_ray, bounces - 1, recursive_record) * k_specular;
      }
      return I_Direct + I_Indirect;

    } else {
        return GetBackgroundColor(ray.GetDirection());
    }
    // }

    // if (record.time < std::numeric_limits<float>::max())
    // {
    //   pixel_color += (I_Direct + I_Indirect);
    //   return pixel_color;
    // }
    // else
    // {
    //   return GetBackgroundColor(ray.GetDirection());
    // }
  }

  glm::vec3 Tracer::GetIDiffuse(glm::vec3 k_diffuse, glm::vec3 dir_to_light, glm::vec3 intensity, glm::vec3 normal) const
  {
    float clamped = std::max(0.0f, glm::dot(dir_to_light, normal));
    glm::vec3 I_Diffuse = clamped * intensity * k_diffuse;
    return I_Diffuse;
  }

  glm::vec3 Tracer::GetISpecular(float shininess, glm::vec3 k_specular, glm::vec3 surface_to_eye, glm::vec3 dir_to_light, glm::vec3 intensity, glm::vec3 normal) const
  {
    glm::vec3 R = glm::normalize(-dir_to_light + 2 * glm::dot(dir_to_light, normal) * normal);
    float clamped = std::max(0.0f, glm::dot(surface_to_eye, R));
    glm::vec3 I_Specular = pow(clamped, shininess) * intensity * k_specular;
    return I_Specular;
  }

  glm::vec3 Tracer::GetBackgroundColor(const glm::vec3 &direction) const
  {
    if (cube_map_ != nullptr)
    {
      return cube_map_->GetTexel(direction);
    }
    else
      return background_color_;
  }

} // namespace GLOO
