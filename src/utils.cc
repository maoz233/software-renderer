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
Mat4 LookAt(Vec3f& eye, Vec3f& center) {
  Vec3f world_up(0.f, 1.f, 0.f);
  Vec3f dir = center - eye;
  dir = dir.Normalize();
  Vec3f right = dir ^ world_up;
  right = right.Normalize();
  Vec3f up = right ^ dir;
  up = up.Normalize();

  Mat4 view_rotate = Mat4::Identity();
  view_rotate[0][0] = right.x;
  view_rotate[0][1] = right.y;
  view_rotate[0][2] = right.z;
  view_rotate[1][0] = up.x;
  view_rotate[1][1] = up.y;
  view_rotate[1][2] = up.z;
  view_rotate[2][0] = -dir.x;
  view_rotate[2][1] = -dir.y;
  view_rotate[2][2] = -dir.z;

  Mat4 view_translate = Mat4::Identity();
  view_translate[0][3] = -eye.x;
  view_translate[1][3] = -eye.y;
  view_translate[2][3] = -eye.z;

  return view_rotate * view_translate;
}

Mat4 OthographicProject(float near, float far, float fov, float aspect_ratio) {
  float half_radian = fov / 360.f * PI;
  float top = std::abs(near) * std::tan(half_radian);
  float bottom = -top;
  float right = top * aspect_ratio;
  float left = -right;

  // Orthographic projection
  Mat4 otho_scale = Mat4::Identity();
  otho_scale[0][0] = 2.f / (right - left);
  otho_scale[1][1] = 2.f / (top - bottom);
  otho_scale[2][2] = 2.f / (near - far);

  Mat4 otho_translate = Mat4::Identity();
  otho_translate[0][3] = -(right + left) / 2.f;
  otho_translate[1][3] = -(top + bottom) / 2.f;
  otho_translate[2][3] = -(near + far) / 2.f;

  return otho_scale * otho_translate;
}

Mat4 PerspectiveProject(float near, float far, float fov, float aspect_ratio) {
  float half_radian = fov / 360.f * PI;
  float top = std::abs(near) * std::tan(half_radian);
  float bottom = -top;
  float right = top * aspect_ratio;
  float left = -right;

  // Orthographic projection
  Mat4 otho_scale = Mat4::Identity();
  otho_scale[0][0] = 2.f / (right - left);
  otho_scale[1][1] = 2.f / (top - bottom);
  otho_scale[2][2] = 2.f / (near - far);

  Mat4 otho_translate = Mat4::Identity();
  otho_translate[0][3] = -(right + left) / 2.f;
  otho_translate[1][3] = -(top + bottom) / 2.f;
  otho_translate[2][3] = -(near + far) / 2.f;

  Mat4 otho = otho_scale * otho_translate;

  // Perspective projection to orthographic projection
  Mat4 persp_2_otho{};
  persp_2_otho[0][0] = near;
  persp_2_otho[1][1] = near;
  persp_2_otho[2][2] = near + far;
  persp_2_otho[2][3] = -near * far;
  persp_2_otho[3][2] = 1.f;

  return otho * persp_2_otho;
}

Mat4 Viewport(float width, float height) {
  Mat4 viewport = Mat4::Identity();

  viewport[0][0] = width / 2.f;
  viewport[0][3] = width / 2.f;
  viewport[1][1] = height / 2.f;
  viewport[1][3] = height / 2.f;

  return viewport;
};
}  // namespace swr
