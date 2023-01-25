/**
 * @file utils.cpp
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "utils.h"

namespace swr {
Mat4& LookAt(Vec3f& origin, Vec3f& target) {
  Vec3f world_up(0.f, 1.f, 0.f);
  Vec3f dir = target - origin;
  Vec3f right = dir ^ world_up;
  Vec3f up = right ^ dir;

  Mat4 translate_view = Mat4::Identity();
  translate_view[0][3] = -origin.x;
  translate_view[1][3] = -origin.y;
  translate_view[2][3] = -origin.z;

  Mat4 roate_view = Mat4::Identity();
  roate_view[0][0] = up.x;
  roate_view[0][1] = up.y;
  roate_view[0][2] = up.z;
  roate_view[1][0] = right.x;
  roate_view[1][1] = right.y;
  roate_view[1][2] = right.z;
  roate_view[2][0] = -dir.x;
  roate_view[2][1] = -dir.y;
  roate_view[2][2] = -dir.z;

  return roate_view * translate_view;
}

Mat4& Project(float near, float far, float theta) {
  float half_radian = theta / 360.f * PI;
  float right = near * std::tan(half_radian);
  float left = -right;
  float top = right;
  float bottom = left;

  // Orthographic projection
  Mat4 otho = Mat4::Identity();
  otho[0][0] = 2.f / (right - left);
  otho[1][1] = 2.f / (top - bottom);
  otho[2][2] = 2.f / (far - near);

  // Perspective projection to orthographic projection
  Mat4 persp_2_otho = Mat4::Identity();
  persp_2_otho[0][0] = near;
  persp_2_otho[1][1] = near;
  persp_2_otho[2][2] = near + far;
  persp_2_otho[2][3] = -near * far;

  return otho * persp_2_otho;
}
}  // namespace swr
