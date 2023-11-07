#include "Plane.hpp"

namespace GLOO {
Plane::Plane(const glm::vec3& normal, float d) {
      d_ = d;
      normal_ = normal;
}

bool Plane::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
  // TODO: Implement ray-plane intersection.
  glm::vec3 Rd = ray.GetDirection();
  glm::vec3 Ro = ray.GetOrigin();

  float t = -(d_ + glm::vec3(normal_, Ro))/(glm::dot(normal_, Rd));

  if (t < t_min) {
      return false;
  }

  if (t < record.time) {
    record.time = t;
    record.normal = glm::normalize(ray.At(t));
    return true;
  }


  return false;
}
}  // namespace GLOO
