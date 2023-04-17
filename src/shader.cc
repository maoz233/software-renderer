/**
 * @file shader.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-02-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "shader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "texture.h"
#include "utils.h"

#if _WIN32
#pragma warning(disable : 4127)
#endif

namespace swr {

void Shader::SetVec2i(Vec2i& vec) { uv_ = vec; }

void Shader::SetVec3f(int name, Vec3f& vec) {
  switch (name) {
    case Vector::VERTEX:
      vertex_ = vec;
      break;
    case Vector::FRAGMENT:
      fragment_ = vec;
      break;
    case Vector::LIGHT:
      light_ = vec;
      break;
    case Vector::EYE:
      eye_ = vec;
      break;
    case Vector::NORMAL:
      normal_ = vec;
      break;
    case Vector::TANGENT:
      tangent_ = vec;
      break;
    case Vector::BITANGENT:
      bitangent_ = vec;
      break;
    default:
      break;
  }
}

void Shader::SetMat4(int name, Mat4& mat) {
  switch (name) {
    case Matrix::MVP:
      mvp_ = mat;
      break;
    default:
      break;
  }
}

void Shader::SetTexture(int name, Texture* texture) {
  switch (name) {
    case ETexture::DIFFUSE_TEXTURE:
      diffuse_texture_ = texture;
      break;
    case ETexture::NORMAL_TEXTURE:
      normal_texture_ = texture;
      break;
    case ETexture::NORMAL_TANGENT_TEXTURE:
      normal_tangent_texture_ = texture;
      break;
    case ETexture::SPECULAR_TEXTURE:
      specular_texture_ = texture;
      break;
    default:
      break;
  }
}

void Shader::Vertex(Vec3f& position) {
  Vec4f homo_vertex{vertex_, 1.f};
  Vec4f v = mvp_ * homo_vertex;

  position = Vec3f(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
}

void Shader::Fragment(uint32_t& pixel) {
  // Diffuse light intensity
  Vec3f pixel_normal = Sample(normal_texture_, uv_.u, uv_.v);
  Vec3f normal = (pixel_normal * 2.f - 255.f).Normalize();

  Vec3f light_dir{light_ - fragment_};

  float diff = std::max(0.f, normal.Normalize() * light_dir.Normalize());

  // Sampling pixel from diffuse texture
  Vec3f pixel_diffuse = Sample(diffuse_texture_, uv_.u, uv_.v);

  pixel = GetColor(pixel_diffuse * diff);
}

// Phong Shading
void PhongShader::Fragment(uint32_t& pixel) {
  // Ambient light intensity
  float ambient = .05f;

  // Diffuse light intensity
  Vec3f pixel_normal = Sample(normal_texture_, uv_.u, uv_.v);
  Vec3f normal = (pixel_normal * 2.f - 255.f).Normalize();

  Vec3f light_dir = (light_ - fragment_).Normalize();

  float diff = std::max(0.f, normal * light_dir);

  // Sampling from diffuse texture
  Vec3f pixel_diffuse = Sample(diffuse_texture_, uv_.u, uv_.v);
  float intensity = ambient + diff;

  // Specular light intensity
  Vec3f reflect = Reflect(light_dir, normal).Normalize();
  Vec3f view_dir = (eye_ - fragment_).Normalize();
  float spec = std::pow(std::max(view_dir * reflect, 0.f), 32.f);

  Vec3f pixel_specular = Sample(specular_texture_, uv_.u, uv_.v);

  pixel = GetColor(pixel_diffuse * intensity + pixel_specular * spec);
}

// Phong Shading wit Normal Mapping in Tangent Space
void NormalMappingShader::Fragment(uint32_t& pixel) {
  // Ambient light intensity
  float ambient = .05f;

  // Diffuse light intensity
  Vec3f pixel_normal = Sample(normal_tangent_texture_, uv_.u, uv_.v);
  Vec3f normal = (pixel_normal * 2.f - 255.f).Normalize();

  // TBN matrix
  Vec3f T = tangent_.Normalize();
  Vec3f N = normal_.Normalize();
  T = (T - N * (T * N)).Normalize();
  Vec3f B_ = N ^ T;

  Mat3 TBN{};
  TBN[0][0] = T.x;
  TBN[1][0] = T.y;
  TBN[2][0] = T.z;
  TBN[0][1] = B_.x;
  TBN[1][1] = B_.y;
  TBN[2][1] = B_.z;
  TBN[0][2] = N.x;
  TBN[1][2] = N.y;
  TBN[2][2] = N.z;

  Vec3f light_pos = TBN * light_;
  Vec3f view_pos = TBN * eye_;
  Vec3f fragment_pos = TBN * fragment_;

  Vec3f light_dir = (light_pos - fragment_pos).Normalize();

  float diff = std::max(0.f, normal * light_dir);

  // Sampling from diffuse texture
  Vec3f pixel_diffuse = Sample(diffuse_texture_, uv_.u, uv_.v);
  float intensity = ambient + diff;

  // Specular light intensity
  Vec3f reflect = Reflect(light_dir, normal).Normalize();
  Vec3f view_dir = (view_pos - fragment_pos).Normalize();
  float spec = std::pow(std::max(view_dir * reflect, 0.f), 32.f);
  Vec3f pixel_specular = Sample(specular_texture_, uv_.u, uv_.v);

  pixel = GetColor(pixel_diffuse * intensity + pixel_specular * spec);
}

Vec3f Sample(Texture* surface, int x, int y) {
  uint8_t* imageData = surface->GetData();

  uint8_t r =
      imageData[4 * ((surface->GetHeight() - y) * surface->GetWidth() + x) + 0];
  uint8_t g =
      imageData[4 * ((surface->GetHeight() - y) * surface->GetWidth() + x) + 1];
  uint8_t b =
      imageData[4 * ((surface->GetHeight() - y) * surface->GetWidth() + x) + 2];

  return Vec3f(r, g, b);
}

Vec3f Reflect(Vec3f& v, Vec3f& normal) {
  return normal * 2.f * (normal * v) - v;
}

uint32_t GetColor(const Vec3f& color) {
  uint32_t R = static_cast<uint32_t>(color.x);
  uint32_t G = static_cast<uint32_t>(color.y);
  uint32_t B = static_cast<uint32_t>(color.z);

  return (255 << 24) | (B << 16) | (G << 8) | R;
}

}  // namespace swr
