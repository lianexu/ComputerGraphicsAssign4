#include "Plane.hpp"

namespace GLOO {
// Plane::Plane(const glm::vec3& normal, float d) {
//       // d_ = d;
//       // normal_ = normal;
// }

bool Plane::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
  // TODO: Implement ray-plane intersection.
  glm::vec3 Rd = ray.GetDirection();
  glm::vec3 Ro = ray.GetOrigin();
  glm::vec3 n = -normal_;
  float t = -(d_ + glm::dot(n, Ro))/(glm::dot(n, Rd));

  if (t < t_min) {
      return false;
  }

  if (t < record.time) {
    record.time = t;
    record.normal = glm::normalize(normal_);
    return true;
  }


  return false;
}
}  // namespace GLOO
