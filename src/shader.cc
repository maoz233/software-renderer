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
    case Vector::LIGHT:
      this->light_ = vec;
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
    case Matrix::MVP_IT:
      this->mvp_it_ = mat;
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
    case Texture::SPECULAR_TEXTURE:
      this->specular_texture_ = texture;
      break;
    default:
      break;
  }
}

void Shader::Vertex(Vec4& position) {
  Vec4 homo_vertex{};
  homo_vertex[0][0] = this->vertex_.x;
  homo_vertex[1][0] = this->vertex_.y;
  homo_vertex[2][0] = this->vertex_.z;
  homo_vertex[3][0] = 1.f;

  Vec4 v = this->mvp_ * homo_vertex;

  position = v;
}

void Shader::Fragment(Uint32& pixel) {
  // Sampling pixel from diffuse texture
  Uint32 p = GetPixel(this->diffuse_texture_, this->uv_.u, this->uv_.v);
  SDL_PixelFormat* format = this->diffuse_texture_->format;

  Uint8 R = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss));
  Uint8 G = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss));
  Uint8 B = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss));

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
  Vec4 homo_normal{};
  homo_normal[0][0] = static_cast<float>(normal_r);
  homo_normal[1][0] = static_cast<float>(normal_g);
  homo_normal[2][0] = static_cast<float>(normal_b);
  homo_normal[3][0] = 0.f;
  Vec4 n = this->mvp_it_ * homo_normal;
  Vec3f normal{n[0][0] / n[3][0], n[1][0] / n[3][0], n[2][0] / n[3][0]};
  float diff =
      std::max(0.f, std::abs(normal.Normalize() * this->light_.Normalize()));

  // Specular light intensity
  Uint32 pixel_specular =
      GetPixel(this->normal_texture_, this->uv_.u, this->uv_.v);
  Uint8 specular_r = 0;
  Uint8 specular_g = 0;
  Uint8 specular_b = 0;
  SDL_GetRGB(pixel_specular, this->normal_texture_->format, &specular_r,
             &specular_g, &specular_b);
  Vec3f r = Reflect(this->light_, normal).Normalize();
  float s =
      std::max(0.f, static_cast<float>(specular_r + specular_g + specular_b));
  float spec = std::pow(std::max(r.z, 0.f), s);

  // Sampling from diffuse texture
  Uint32 p = GetPixel(this->diffuse_texture_, this->uv_.u, this->uv_.v);
  SDL_PixelFormat* format = this->diffuse_texture_->format;

  // R, G, B channels
  Uint8 R = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) *
      (ambient + diff + spec));
  Uint8 G = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) *
      (ambient + diff + spec));
  Uint8 B = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) *
      (ambient + diff + spec));

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
}  // namespace swr
