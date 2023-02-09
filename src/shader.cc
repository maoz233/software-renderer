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
#include <vector>

#include "SDL.h"
#include "utils.h"

#if _WIN32
#pragma warning(disable : 4127)
#endif

namespace swr {

void VertexUniform::SetVec3f(Vec3f& vec) { this->vertex = vec; }

void VertexUniform::SetMat4(int name, Mat4& mat) {
  switch (name) {
    case Matrix::VIEWPORT:
      this->viewport = mat;
      break;
    case Matrix::PROJECTION:
      this->projection = mat;
      break;
    case Matrix::VIEW:
      this->view = mat;
      break;
    default:
      break;
  }
}

void FragmentUniform::SetVec2i(Vec2i& vec) { this->uv = vec; }

void FragmentUniform::SetVec3f(int name, Vec3f& vec) {
  switch (name) {
    case Vector::LIGHT:
      this->light = vec;
      break;
    case Vector::NORMAL:
      this->normal = vec;
      break;
    default:
      break;
  }
}

void FragmentUniform::SetTexture(int name, SDL_Surface* texture) {
  switch (name) {
    case Texture::DIFFUSE_TEXTURE:
      this->diffuse_texture = texture;
      break;
    default:
      break;
  }
}

void Shader::Vertex(VertexUniform& uniform, Vec3f& position) {
  Vec4 homo_vertex{};
  homo_vertex[0][0] = uniform.vertex.x;
  homo_vertex[1][0] = uniform.vertex.y;
  homo_vertex[2][0] = uniform.vertex.z;
  homo_vertex[3][0] = 1.f;

  Vec4 v = uniform.viewport * uniform.projection * uniform.view * homo_vertex;
  position = Vec3f(v[0][0] / v[3][0], v[1][0] / v[3][0], v[2][0] / v[3][0]);
}

// Flat Shading
void Shader::Fragment(FragmentUniform& uniform, Uint32& pixel) {
  // Intensity
  float intensity = 0.75f;
  // Discard
  if (intensity <= 0.f) {
    return;
  }

  // Sampling from diffuse texture
  Uint32 p = GetPixel(uniform.diffuse_texture, uniform.uv.u, uniform.uv.v);
  SDL_PixelFormat* format = uniform.diffuse_texture->format;

  // Diffuse
  Uint8 R = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) * intensity);
  Uint8 G = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) * intensity);
  Uint8 B = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) * intensity);

  pixel = SDL_MapRGB(format, R, G, B);
}

// Blinn-Phong Shading
void BlinnPhongShader::Fragment(FragmentUniform& uniform, Uint32& pixel) {
  // Intensity
  float intensity =
      std::max(uniform.light.Normalize() * uniform.normal.Normalize(), 0.f);

  // Discard
  if (intensity <= 0.f) {
    return;
  }

  // Sampling from diffuse texture
  Uint32 p = GetPixel(uniform.diffuse_texture, uniform.uv.u, uniform.uv.v);
  SDL_PixelFormat* format = uniform.diffuse_texture->format;

  // Ambient
  Uint8 R_ambient = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) * .05f);
  Uint8 G_ambient = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) * .05f);
  Uint8 B_ambient = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) * .05f);

  // Diffsuse
  Uint8 R_diff = static_cast<Uint8>(
      (((p & format->Rmask) >> format->Rshift) << format->Rloss) * intensity);
  Uint8 G_diff = static_cast<Uint8>(
      (((p & format->Gmask) >> format->Gshift) << format->Gloss) * intensity);
  Uint8 B_diff = static_cast<Uint8>(
      (((p & format->Bmask) >> format->Bshift) << format->Bloss) * intensity);

  // Specular

  pixel = SDL_MapRGB(format, R_ambient + R_diff, G_ambient + G_diff,
                     B_ambient + B_diff);
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
