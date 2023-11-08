#include "Triangle.hpp"

#include <iostream>
#include <stdexcept>

#include <glm/common.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Plane.hpp"

namespace GLOO {
Triangle::Triangle(const glm::vec3& p0,
                   const glm::vec3& p1,
                   const glm::vec3& p2,
                   const glm::vec3& n0,
                   const glm::vec3& n1,
                   const glm::vec3& n2) {
  positions_.push_back(p0);
  positions_.push_back(p1);
  positions_.push_back(p2);
  normals_.push_back(n0);
  normals_.push_back(n1);
  normals_.push_back(n2);
}

Triangle::Triangle(const std::vector<glm::vec3>& positions,
                   const std::vector<glm::vec3>& normals) {
  positions_ = positions;
  normals_ = normals;
}

bool Triangle::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
  // TODO: Implement ray-triangle intersection.

  // Solve for x in Ax = b
  float ax = positions_[0].x;
  float ay = positions_[0].y;
  float az = positions_[0].z;

  float bx = positions_[1].x;
  float by = positions_[1].y;
  float bz = positions_[1].z;

  float cx = positions_[2].x;
  float cy = positions_[2].y;
  float cz = positions_[2].z;

  glm::vec3 d = ray.GetDirection();
  float dx = d.x;
  float dy = d.y;
  float dz = d.z;

  glm::vec3 O = ray.GetOrigin();
  float ox = O.x;
  float oy = O.y;
  float oz = O.z;

  glm::mat3 A(0.f);

  A[0][0] = ax-bx;
  A[0][1] = ax-cx;
  A[0][2] = dx;

  A[1][0] = ay-by;
  A[1][1] = ay-cy;
  A[1][2] = dy;

  A[2][0] = az-bz;
  A[2][1] = az-cz;
  A[2][2] = dz;

  A = glm::transpose(A);

  glm::vec3 b(ax-ox, ay-oy, az-oz);

  glm::vec3 x = glm::inverse(A) * b;
  float beta = x[0];
  float gamma = x[1];
  float alpha = 1-beta-gamma;
  float t = x[2];

  if ((beta + gamma <= 1) && (beta >= 0) && (gamma >= 0) && t > t_min){
    if (t < record.time){
      record.time = t;
      record.normal = glm::normalize(alpha * normals_[0] + beta * normals_[1] + gamma * normals_[2]);
      return true;
    }
  }

  return false;

}
}  // namespace GLOO
