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
#include <vector>

#include "SDL.h"
#include "utils.h"

#if _WIN32
#pragma warning(disable : 4127)
#endif

namespace swr {

void Shader::SetVec2i(Vec2i& vec) { this->uv_ = vec; }

void Shader::SetVec3f(int name, Vec3f& vec) {
  switch (name) {
    case Vector::VERTEX:
      this->vertex_ = vec;
      break;
    case Vector::FRAGMENT:
      this->fragment_ = vec;
      break;
    case Vector::LIGHT:
      this->light_ = vec;
      break;
    case Vector::EYE:
      this->eye_ = vec;
      break;
    case Vector::NORMAL:
      this->normal_ = vec;
      break;
    case Vector::TANGENT:
      this->tangent_ = vec;
      break;
    case Vector::BITANGENT:
      this->bitangent_ = vec;
      break;
    default:
      break;
  }
}

void Shader::SetMat4(int name, Mat4& mat) {
  switch (name) {
    case Matrix::MVP:
      this->mvp_ = mat;
      break;
    default:
      break;
  }
}

void Shader::SetTexture(int name, SDL_Surface* texture) {
  switch (name) {
    case Texture::DIFFUSE_TEXTURE:
      this->diffuse_texture_ = texture;
      break;
    case Texture::NORMAL_TEXTURE:
      this->normal_texture_ = texture;
      break;
    case Texture::NORMAL_TANGENT_TEXTURE:
      this->normal_tangent_texture_ = texture;
      break;
    case Texture::SPECULAR_TEXTURE:
      this->specular_texture_ = texture;
      break;
    default:
      break;
  }
}

void Shader::Vertex(Vec3f& position) {
  Vec4f homo_vertex{this->vertex_, 1.f};
  Vec4f v = this->mvp_ * homo_vertex;

  position = Vec3f(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
}

void Shader::Fragment(Uint32& pixel) {
  // Diffuse light intensity
  Uint32 pixel_normal =
      GetPixel(this->normal_texture_, this->uv_.u, this->uv_.v);
  Uint8 normal_r = 0;
  Uint8 normal_g = 0;
  Uint8 normal_b = 0;
  SDL_GetRGB(pixel_normal, this->normal_texture_->format, &normal_r, &normal_g,
             &normal_b);

  Vec3f normal = Vec3f{static_cast<float>(normal_r) * 2.f - 255.f,
                       static_cast<float>(normal_g) * 2.f - 255.f,
                       static_cast<float>(normal_b) * 2.f - 255.f}
                     .Normalize();

  Vec3f light_dir{this->light_ - this->fragment_};

  float diff = std::max(0.f, normal.Normalize() * light_dir.Normalize());

  // Sampling pixel from diffuse texture
  Uint32 p = GetPixel(this->diffuse_texture_, this->uv_.u, this->uv_.v);
  SDL_PixelFormat* format = this->diffuse_texture_->format;

  Uint8 R = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) * diff);
  Uint8 G = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) * diff);
  Uint8 B = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) * diff);

  pixel = SDL_MapRGB(format, R, G, B);
}

// Phong Shading
void PhongShader::Fragment(Uint32& pixel) {
  // Ambient light intensity
  float ambient = .05f;

  // Diffuse light intensity
  Uint32 pixel_normal =
      GetPixel(this->normal_texture_, this->uv_.u, this->uv_.v);
  Uint8 normal_r = 0;
  Uint8 normal_g = 0;
  Uint8 normal_b = 0;
  SDL_GetRGB(pixel_normal, this->normal_texture_->format, &normal_r, &normal_g,
             &normal_b);

  Vec3f normal = Vec3f{static_cast<float>(normal_r) * 2.f - 255.f,
                       static_cast<float>(normal_g) * 2.f - 255.f,
                       static_cast<float>(normal_b) * 2.f - 255.f}
                     .Normalize();

  Vec3f light_dir = (this->light_ - this->fragment_).Normalize();

  float diff = std::max(0.f, normal * light_dir);

  // Specular light intensity
  Uint32 pixel_specular =
      GetPixel(this->normal_texture_, this->uv_.u, this->uv_.v);
  Uint8 specular_r = 0;
  Uint8 trash = 0;
  SDL_GetRGB(pixel_specular, this->normal_texture_->format, &specular_r, &trash,
             &trash);
  float s = std::max(0.f, static_cast<float>(specular_r));
  Vec3f reflect = Reflect(light_dir, normal).Normalize();
  Vec3f view_dir = (this->eye_ - this->fragment_).Normalize();
  float spec = std::pow(std::max(view_dir * reflect, 0.f), s);

  // Sampling from diffuse texture
  Uint32 p = GetPixel(this->diffuse_texture_, this->uv_.u, this->uv_.v);
  SDL_PixelFormat* format = this->diffuse_texture_->format;

  float intensity = ambient + diff + spec;
  // R, G, B channels
  Uint8 R = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) * intensity);
  Uint8 G = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) * intensity);
  Uint8 B = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) * intensity);

  pixel = SDL_MapRGB(format, R, G, B);
}

// Phong Shading wit Normal Mapping in Tangent Space
void NormalMappingShader::Fragment(Uint32& pixel) {
  // Ambient light intensity
  float ambient = .05f;

  // Diffuse light intensity
  Uint32 pixel_normal =
      GetPixel(this->normal_tangent_texture_, this->uv_.u, this->uv_.v);
  Uint8 normal_r = 0;
  Uint8 normal_g = 0;
  Uint8 normal_b = 0;
  SDL_GetRGB(pixel_normal, this->normal_tangent_texture_->format, &normal_r,
             &normal_g, &normal_b);

  Vec3f normal = Vec3f{static_cast<float>(normal_r) * 2.f - 255.f,
                       static_cast<float>(normal_g) * 2.f - 255.f,
                       static_cast<float>(normal_b) * 2.f - 255.f}
                     .Normalize();

  // TBN matrix
  Vec3f N = this->normal_.Normalize();
  Vec3f T = (T - N * (T * N)).Normalize();
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

  Vec3f light_pos = TBN * this->light_;
  Vec3f view_pos = TBN * this->eye_;
  Vec3f fragment_pos = TBN * this->fragment_;

  Vec3f light_dir = (light_pos - fragment_pos).Normalize();

  float diff = std::max(0.f, normal * light_dir);

  // Specular light intensity
  Uint32 pixel_specular =
      GetPixel(this->normal_texture_, this->uv_.u, this->uv_.v);
  Uint8 specular_r = 0;
  Uint8 trash = 0;
  SDL_GetRGB(pixel_specular, this->normal_texture_->format, &specular_r, &trash,
             &trash);
  float s = std::max(0.f, static_cast<float>(specular_r));
  Vec3f reflect = Reflect(light_dir, normal).Normalize();
  Vec3f view_dir = (view_pos - fragment_pos).Normalize();
  float spec = std::pow(std::max(view_dir * reflect, 0.f), s);

  // Sampling from diffuse texture
  Uint32 p = GetPixel(this->diffuse_texture_, this->uv_.u, this->uv_.v);
  SDL_PixelFormat* format = this->diffuse_texture_->format;

  float intensity = ambient + diff + spec;
  // R, G, B channels
  Uint8 R = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) * intensity);
  Uint8 G = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) * intensity);
  Uint8 B = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) * intensity);

  pixel = SDL_MapRGB(format, R, G, B);
}

Uint32 GetPixel(SDL_Surface* surface, int x, int y) {
  // flip surface vertically
  y = surface->h - y;
  // avoid coordinate beyond surface
  if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
    return SDL_MapRGB(surface->format, 255, 255, 255);
  }

  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp) {
    case 1:
      return *p;

    case 2:
      return *(Uint16*)p;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
      return *(Uint32*)p;

    default:
      return 0; /* shouldn't happen, but avoids warnings */
  }
}

Vec3f Reflect(Vec3f& v, Vec3f& normal) {
  return normal * 2.f * (normal * v) - v;
}

}  // namespace swr
